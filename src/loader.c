#include "loader.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "shm_data.h"
#include "hash.h"
#include "spin_lock.h"

#define MAGIC         "MONITOR"
#define VERSION_HIGH  1
#define VERSION_LOW   0
#define SHM_KEY       0xABCD0605   
#define SHM_CAPCITY   10*1024*1024 // 10MB
#define OFFSET        1024         // 上报数组偏移

int             moni_status           = 0;
void*           moni_shmaddr          = NULL;
int             moni_instance_id      = 0;
int             moni_capcity          = SHM_CAPCITY;
char            moni_base_name[MAX_PATH_LEN];
moni_hash_map_t moni_metric_index_map;

#ifndef NDEBUG
extern uint64_t hash_map_set_conflics;
extern uint64_t hash_map_set_count;
extern uint64_t hash_map_get_conflics;
extern uint64_t hash_map_get_count;
extern uint64_t hash_map_lock_calls;
extern uint64_t shm_lock_calls;
extern uint64_t hash_map_lock_spins;
extern uint64_t hash_map_lock_max_spins;
extern uint64_t shm_lock_spins;
extern uint64_t shm_lock_max_spins;
#endif

static int __moni_check_shm_metric(moni_hash_key_t key, moni_hash_value_t value)
{
    moni_entry_t* entry = moni_get_entry(moni_shmaddr, value);
    return !strncmp((char*)entry->data.metric, key, sizeof(entry->data.metric)-1);
}

void moni_load()
{
    char         path[256];
    int          shmid;
    moni_head_t* head;
    int          linksize;
    int          pos;

    linksize = readlink("/proc/self/exe", path, sizeof(path));
    if (linksize == -1) return;
    path[linksize] = '\0';
    moni_bkdr_hash(path, &moni_instance_id); // 路径名映射为整数标识
    if (!moni_instance_id) return;

    for (pos = linksize-1; pos >=0; pos--) {
        if (path[pos] == '/') { 
            strcpy(moni_base_name, &path[pos+1]);
            path[pos+1] = '\0';
            break;
        }
    }
    if (!strlen(moni_base_name)) return;
    fprintf(stderr, "info: process image path: %s, base_name: %s, inst: %u\n", 
            path, moni_base_name, moni_instance_id);
    
    shmid = shmget(SHM_KEY, SHM_CAPCITY, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1 && errno != EEXIST) {
        fprintf(stderr, "error: shmget failed.\n");
        return;
    }

    // 其他进程已经创建过共享内存
    if (errno == EEXIST) {
        // 打开已经存在的共享内存
        shmid = shmget(SHM_KEY, SHM_CAPCITY, 0666); 
        if (shmid == -1) {
            fprintf(stderr, "error: shmget failed, errno: %d.\n", errno);
            return;
        }
        moni_shmaddr = shmat(shmid, NULL, 0);
        if (moni_shmaddr == (void*)-1) {
            fprintf(stderr, "error: shmat failed, shmid: %d, errno: %d.\n", shmid, errno);
            return;
        }
        fprintf(stderr, "info: shm for monitor api already exist, id: %d.\n", shmid);
        if (moni_get_head(moni_shmaddr)->version != moni_make_version(VERSION_HIGH, VERSION_LOW)) {
            // api版本与共享内存上报数据版本不一致，打印警告信息
            fprintf(stderr, "warn: incompatible monitor version, api version: %d.%d, "
                    "please remove shm for key 0x%X if neccesary.\n", VERSION_HIGH, VERSION_LOW, SHM_KEY);
        }
        goto SUCC;
    }

    // 第一次创建本机上的上共享内存区
    moni_shmaddr = (moni_head_t*)shmat(shmid, NULL, 0);
    if (moni_shmaddr == (void*)-1) {
        fprintf(stderr, "error: shmat failed, shmid: %d, errno: %d.\n", shmid, errno);
        return;
    }
    head = moni_get_head(moni_shmaddr);
    fprintf(stderr, "info: shm for monitor api init for first time.\n");
    bzero(head, SHM_CAPCITY);
    strncpy((char*)head->magic, MAGIC, strlen(MAGIC));
    head->lock = UNLOCK;
    head->version = moni_make_version(VERSION_HIGH, VERSION_LOW);
    head->capcity = SHM_CAPCITY;
    head->offset = OFFSET;
    head->entries = 0;
SUCC:
    moni_status = 1;
    moni_hash_map_init(&moni_metric_index_map, __moni_check_shm_metric); 
    return;
}

void moni_unload()
{
#ifndef NDEBUG
    int i;
    int hash_map_used = 0;
    for (i = 0; i < BUCKET_NUM; i++) {
        if (moni_metric_index_map.bucket[i] != RAW) {
            hash_map_used++;
        }
    }
    fprintf(stderr, "********* debug info *********\n");
    fprintf(stderr, "hmap used: %d\n", hash_map_used);
    fprintf(stderr, "hmap get: %lu conflics: %lu rate: %f%%\n", 
            hash_map_get_count, hash_map_get_conflics, hash_map_get_conflics*100.0f/hash_map_get_count);
    fprintf(stderr, "hmap set: %lu conflics: %lu rate: %f%%\n", 
            hash_map_set_count, hash_map_set_conflics, hash_map_set_conflics*100.0f/hash_map_set_count);
    fprintf(stderr, "hmap lock spin calls: %lu spins: %lu max: %lu avg: %f\n", 
            hash_map_lock_calls, hash_map_lock_spins, hash_map_lock_max_spins,
            hash_map_lock_calls ? hash_map_lock_spins*1.0f/hash_map_lock_calls : 0);
    fprintf(stderr, "shm  lock spin calls: %lu spins: %lu max: %lu avg: %f\n", 
            shm_lock_calls, shm_lock_spins, shm_lock_max_spins,
            shm_lock_calls ? shm_lock_spins*1.0f/shm_lock_calls : 0);
    fprintf(stderr, "********* debug info *********\n");
#endif
    fprintf(stderr, "info: detach shm.\n");
    shmdt(moni_shmaddr);
}

