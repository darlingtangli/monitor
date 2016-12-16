#include "report.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include "loader.h"
#include "shm_data.h"
#include "spin_lock.h"
#include "atomic.h"
#include "hash.h"
#include "hash_map.h"

static moni_spin_lock_t __lock = UNLOCK;

// 为进程上报的属性找出其的entry号
static uint32_t __moni_get_entry_index(const char* metric, moni_attr_t type)
{
    int  i;
    uint32_t index;
    moni_head_t* head;
    moni_entry_t* entry;
    moni_entry_t* tail;

    i = moni_hash_map_get(&moni_metric_index_map, metric);
    if (i >= 0) return i;

    // moni_metric_index_map中没有要的index，遍历共享内存的entry表找出来
    index = UINT_MAX;
    head = moni_get_head(moni_shmaddr);
    for (i = 0; i < head->entries; i++) {
        entry = moni_get_entry(moni_shmaddr, i);
        if (entry->data.instance_id == moni_instance_id && 
                strncmp((char*)entry->data.metric, metric, sizeof(entry->data.metric)-1) == 0) {
            index = i;
            // lock local process map
            moni_spin_lock(&__lock);
            moni_hash_map_set(&moni_metric_index_map, metric, index);
            moni_spin_unlock(&__lock);
            break;
        }
    }
    if (index != UINT_MAX) return index;

    // 共享内存里也没有，需要新建
    index = UINT_MAX;
    // lock shm
    moni_spin_lock(&head->lock);
    do {
        // double check
        for (i = 0; i < head->entries; i++) {
            entry = moni_get_entry(moni_shmaddr, i);
            if (entry->data.instance_id == moni_instance_id && 
                    strncmp((char*)entry->data.metric, metric, sizeof(entry->data.metric)-1) == 0) {
                index = i;
                // because of shm lock, no need lock local process map
                moni_hash_map_set(&moni_metric_index_map, metric, index);
                break;
            }
        }
        // 其他线程已经新建了entry
        if (index != UINT_MAX) break;
        // 新建entry
        tail = moni_get_entry(moni_shmaddr, head->entries);
        if ((char*)(tail+1)-(char*)moni_shmaddr > moni_capcity) {
            // 共享内存不够用了
            fprintf(stderr, "warn: no more space for monoitor report data in shm.\n");
            index = UINT_MAX;
            break;
        }
        bzero(tail, sizeof(*tail));
        index = head->entries;
        head->entries++;
        moni_hash_map_set(&moni_metric_index_map, metric, index);
        tail->data.type = type;
        tail->data.instance_id = moni_instance_id;
        strncpy((char*)tail->data.metric, metric, sizeof(tail->data.metric));
        tail->data.metric[sizeof(tail->data.metric)-1] = '\0';
        if (type == AT_CALL) tail->data.record.call.cost_min_us = UINT_MAX;
        if (type == AT_MIN) tail->data.record.min = -1;
    } while (0);
    moni_spin_unlock(&head->lock);

    return index;
}

uint64_t moni_timer()
{
    // NOTE: 使用gettimeofday来计时的原因：1. 精度足够；2. 高效；3. 线程安全
    // 历史上gettimeofday似乎并不是线程安全的，并且系统调用很慢，common库中的inv_timeprovider使用了锁来保证线程安全，
    // 在Linux系统上现在已无必要。
    // http://www.cppblog.com/Solstice/archive/2014/08/21/139769.html
    // http://adam8157.info/blog/2011/10/linux-vdso/
    // http://www.ibm.com/developerworks/cn/linux/kernel/l-k26ncpu/index.html
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000UL + tv.tv_usec;
}

void moni_report_call(const char* metric,  
        const char* caller,          
        const char* callee,          
        moni_call_status_t status,                  
        uint64_t cost_us)  
{
    if (!moni_status) return;

    uint32_t index = __moni_get_entry_index(metric, AT_CALL);
    if (index == UINT_MAX) return;

    // 主被调服务名如果已经写入就不再写了
    // 根据首字节有没有内容判断是不是已写入
    moni_entry_t* entry = moni_get_entry(moni_shmaddr, index);
    entry->data.type = AT_CALL;
    // caller
    if (!entry->data.record.call.caller[0])
    {
        strncpy((char*)entry->data.record.call.caller, 
                strlen(caller) ? caller : moni_base_name,  // caller为空串则用进程映像文件名代替
                sizeof(entry->data.record.call.caller));
        entry->data.record.call.caller[sizeof(entry->data.record.call.caller)-1] = '\0';
    }
    // callee
    if (!entry->data.record.call.callee[0])
    {
        strncpy((char*)entry->data.record.call.callee, callee, sizeof(entry->data.record.call.callee));
        entry->data.record.call.callee[sizeof(entry->data.record.call.callee)-1] = '\0';
    }

    // NOTE: 由于api和agent更新count,succ和exception字段的操作并不是原子的，可能会出现
    //       数据不一致的情况，比如在并发量大的情况下会出现succ+exception>count的情况，
    //       保证强一致需要加锁，但考虑到效率问题，这里并没有加锁，而是采取折中方案：
    //       agent读取数据时如果发现succ+exception>count则增大count数值保证
    //       succ+exception<=succ，这样虽然牺牲了上报的准确性，但考虑到概率问题及程度问题
    //       这种折中还是可以接受的
    
    // count
    moni_atomic_add(&entry->data.record.call.count, 1);
    // succ exception
    switch (status)
    {
        case MCS_SUCC:
        {
            moni_atomic_add(&entry->data.record.call.succ, 1);
            break;
        }
        case MCS_EXCEPTION:
        {
            moni_atomic_add(&entry->data.record.call.exception, 1);
            break;
        }
        case MCS_FAILED:
        {
            // nothing to do 
            // failed = count - succ - exception
            break;
        }
    }
    // cost_us
    moni_atomic_add(&entry->data.record.call.cost_us, cost_us);
    // cost_min_us
    if (cost_us < entry->data.record.call.cost_min_us)
    {
        entry->data.record.call.cost_min_us = cost_us;
    }
    // cost_max_us
    if (cost_us > entry->data.record.call.cost_max_us)
    {
        entry->data.record.call.cost_max_us = cost_us;
    }

    return;
}

void moni_report_incr(const char* metric, uint64_t step)
{
    if (!moni_status) return;

    uint32_t index = __moni_get_entry_index(metric, AT_INCR);
    if (index == UINT_MAX) return;

    moni_entry_t* entry = moni_get_entry(moni_shmaddr, index);
    entry->data.type = AT_INCR;
    moni_atomic_add(&entry->data.record.incr, step);

    return;
}

void moni_report_statics(const char* metric, uint64_t value)
{
    if (!moni_status) return;

    uint32_t index = __moni_get_entry_index(metric, AT_STATICS);
    if (index == UINT_MAX) return;

    moni_entry_t* entry = moni_get_entry(moni_shmaddr, index);
    entry->data.type = AT_STATICS;
    entry->data.record.statics = value;

    return;
}

void moni_report_avg(const char* metric, uint64_t value)
{
    if (!moni_status) return;

    uint32_t index = __moni_get_entry_index(metric, AT_AVG);
    if (index == UINT_MAX) return;

    moni_entry_t* entry = moni_get_entry(moni_shmaddr, index);
    entry->data.type = AT_AVG;
    moni_atomic_add(&entry->data.record.avg.sum, value);
    moni_atomic_add(&entry->data.record.avg.count, 1);

    return;
}

void moni_report_min(const char* metric, uint64_t value)
{
    if (!moni_status) return;

    uint32_t index = __moni_get_entry_index(metric, AT_MIN);
    if (index == UINT_MAX) return;

    moni_entry_t* entry = moni_get_entry(moni_shmaddr, index);
    entry->data.type = AT_MIN;
    if (value < entry->data.record.min) entry->data.record.min = value;

    return;
}

void moni_report_max(const char* metric, uint64_t value)
{
    if (!moni_status) return;

    uint32_t index = __moni_get_entry_index(metric, AT_MAX);
    if (index == UINT_MAX) return;

    moni_entry_t* entry = moni_get_entry(moni_shmaddr, index);
    entry->data.type = AT_MAX;
    if (value > entry->data.record.max) entry->data.record.max = value;

    return;
}

void moni_simple_hash(const char* str, char* buf, int len)
{
    uint32_t hash;
    moni_bkdr_hash(str, &hash);
    snprintf(buf, len, "%d", hash % 999983);

    return;
}

const char* moni_process_image_name()
{
    return moni_base_name;
}

const char* moni_version()
{
    return "1.0";
}
