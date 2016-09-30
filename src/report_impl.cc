#include "report.h"

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>
#include <sstream>
#include "initiator.h"
#include "shm_data.h"
#include "simple_spin_lock.h"
#include "atomic.h"
#include "hash.h"
#include "hash_map.h"

namespace inv 
{

namespace monitor
{

extern const char* VERSION;

// 静态对象的初始化在main函数之前，确保调用api的进程在main函数前完成api的初始化
static Initiator initiator;

// 为进程上报的属性找出其的entry号
static uint32_t GetEntryIndex(const std::string& metric, AttrType type)
{
    assert(initiator.status);
    // 保存进程上报属性的entry
    static HashMap metric_index_map(initiator.shmaddr);
    static SimpleSpinLock::LockState metric_index_map_lock = SimpleSpinLock::UNLOCK;

    int i = metric_index_map.Get(metric);
    if (i >= 0) return i;

    // metric_index_map中没有要的index，遍历共享内存的entry表找出来
    uint32_t index = UINT_MAX;
    Head* head = GetHead(initiator.shmaddr);
    {
        // lock local process map
        LockGuard lg(&metric_index_map_lock);
        for (uint32_t i = 0; i < head->entries; i++)
        {
            Entry* entry = GetEntry(initiator.shmaddr, i);
            if (entry->data.instance_id == initiator.instance_id && 
                    strncmp((char*)entry->data.metric, metric.c_str(), sizeof(entry->data.metric)-1) == 0) 
            {
                index = i;
                metric_index_map.Set(metric, index);
                break;
            }
        }
        if (index != UINT_MAX) return index;
    }

    // 共享内存里也没有，需要新建
    {
        // lock shm
        LockGuard lg(&head->lock);
        // double check
        for (uint32_t i = 0; i < head->entries; i++)
        {
            Entry* entry = GetEntry(initiator.shmaddr, i);
            if (entry->data.instance_id == initiator.instance_id && 
                    strncmp((char*)entry->data.metric, metric.c_str(), sizeof(entry->data.metric)-1) == 0) 
            {
                index = i;
                metric_index_map.Set(metric, index);
                break;
            }
        }
        if (index != UINT_MAX) return index;
        // 新建entry
        Entry* tail = GetEntry(initiator.shmaddr, head->entries);
        if ((char*)(tail+1)-(char*)initiator.shmaddr > initiator.capcity) 
        {
            // 共享内存不够用了
            fprintf(stderr, "warn: no more space for monoitor report data in shm.\n");
            return UINT_MAX;
        }
        bzero(tail, sizeof(*tail));
        index = head->entries;
        head->entries++;
        metric_index_map.Set(metric, index);
        tail->data.type = type;
        tail->data.instance_id = initiator.instance_id;
        strncpy((char*)tail->data.metric, metric.c_str(), sizeof(tail->data.metric));
        tail->data.metric[sizeof(tail->data.metric)-1] = '\0';
        if (type == AT_CALL) tail->data.record.call.cost_min_us = UINT_MAX;
        if (type == AT_MIN) tail->data.record.min = uint64_t(-1);
    }

    return index;
}

uint64_t Timer()
{
    // NOTE: 使用gettimeofday来计时的原因：1. 精度足够；2. 高效；3. 线程安全
    // 历史上gettimeofday似乎并不是线程安全的，并且系统调用很慢，common库中的inv_timeprovider使用了锁来保证线程安全，
    // 在Linux系统上现在已无必要。
    // http://www.cppblog.com/Solstice/archive/2014/08/21/139769.html
    // http://adam8157.info/blog/2011/10/linux-vdso/
    // http://www.ibm.com/developerworks/cn/linux/kernel/l-k26ncpu/index.html
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return uint64_t(tv.tv_sec)*1000000 + tv.tv_usec;
}

void ReportCall(const std::string& metric,  
        const std::string& caller,          
        const std::string& callee,          
        CallStatus status,                  
        uint64_t cost_us)  
{
    if (!initiator.status) return;

    uint32_t index = GetEntryIndex(metric, AT_CALL);
    if (index == UINT_MAX) return;

    // 主被调服务名如果已经写入就不再写了
    // 根据首字节有没有内容判断是不是已写入
    Entry* entry = GetEntry(initiator.shmaddr, index);
    entry->data.type = AT_CALL;
    // caller
    if (!entry->data.record.call.caller[0])
    {
        strncpy((char*)entry->data.record.call.caller, 
                caller.length() ? caller.c_str() : initiator.base_name.c_str(),  // caller为空串则用进程映像文件名代替
                sizeof(entry->data.record.call.caller));
        entry->data.record.call.caller[sizeof(entry->data.record.call.caller)-1] = '\0';
    }
    // callee
    if (!entry->data.record.call.callee[0])
    {
        strncpy((char*)entry->data.record.call.callee, callee.c_str(), sizeof(entry->data.record.call.callee));
        entry->data.record.call.callee[sizeof(entry->data.record.call.callee)-1] = '\0';
    }
    // count
    Atomic::Add(&entry->data.record.call.count, uint32_t(1));
    // succ exception
    switch (status)
    {
        case CS_SUCC:
        {
            Atomic::Add(&entry->data.record.call.succ, uint32_t(1));
            break;
        }
        case CS_EXCEPTION:
        {
            Atomic::Add(&entry->data.record.call.exception, uint32_t(1));
            break;
        }
        case CS_FAILED:
        {
            // nothing to do 
            // failed = count - succ - exception
            break;
        }
    }
    // cost_us
    Atomic::Add(&entry->data.record.call.cost_us, cost_us);
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

void ReportIncr(const std::string& metric, uint64_t step)
{
    if (!initiator.status) return;

    uint32_t index = GetEntryIndex(metric, AT_INCR);
    if (index == UINT_MAX) return;

    Entry* entry = GetEntry(initiator.shmaddr, index);
    entry->data.type = AT_INCR;
    Atomic::Add(&entry->data.record.incr, step);

    return;
}

void ReportStatics(const std::string& metric, uint64_t value)
{
    if (!initiator.status) return;

    uint32_t index = GetEntryIndex(metric, AT_STATICS);
    if (index == UINT_MAX) return;

    Entry* entry = GetEntry(initiator.shmaddr, index);
    entry->data.type = AT_STATICS;
    entry->data.record.statics = value;

    return;
}

void ReportAvg(const std::string& metric, uint64_t value)
{
    if (!initiator.status) return;

    uint32_t index = GetEntryIndex(metric, AT_AVG);
    if (index == UINT_MAX) return;

    Entry* entry = GetEntry(initiator.shmaddr, index);
    entry->data.type = AT_AVG;
    Atomic::Add(&entry->data.record.avg.sum, value);
    Atomic::Add(&entry->data.record.avg.count, uint64_t(1));

    return;
}

void ReportMin(const std::string& metric, uint64_t value)
{
    if (!initiator.status) return;

    uint32_t index = GetEntryIndex(metric, AT_MIN);
    if (index == UINT_MAX) return;

    Entry* entry = GetEntry(initiator.shmaddr, index);
    entry->data.type = AT_MIN;
    if (value < entry->data.record.min) entry->data.record.min = value;

    return;
}

void ReportMax(const std::string& metric, uint64_t value)
{
    if (!initiator.status) return;

    uint32_t index = GetEntryIndex(metric, AT_MAX);
    if (index == UINT_MAX) return;

    Entry* entry = GetEntry(initiator.shmaddr, index);
    entry->data.type = AT_MAX;
    if (value > entry->data.record.max) entry->data.record.max = value;

    return;
}

std::string SimpleHash(const std::string& str)
{
    std::stringstream ss;
    ss << BKDRHash(str.c_str()) % 999983;
    return ss.str();
}

std::string ProcessImageName()
{
    return initiator.base_name;
}

std::string Version()
{
    return VERSION;
}

} // namespace monitor

} // namespace inv

