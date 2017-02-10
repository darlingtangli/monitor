#include "spin_lock.h"

#include <stdio.h>
#include <sched.h>

#ifndef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include "atomic.h"
#include "loader.h"
#include "shm_data.h"
uint64_t hash_map_lock_calls     = 0;
uint64_t shm_lock_calls          = 0;
uint64_t hash_map_lock_spins     = 0;
uint64_t hash_map_lock_max_spins = 0;
uint64_t shm_lock_spins          = 0;
uint64_t shm_lock_max_spins      = 0;
void*    hash_map_lock           = NULL;
void*    shm_lock                = NULL;
#endif

// acquire
void moni_spin_lock(moni_spin_lock_t* lock) {
    uint64_t cnt = 1;
#ifndef NDEBUG
    moni_head_t *head;
    uint64_t *spins = NULL;
    uint64_t *calls = NULL;
    if (!shm_lock) {
        head = moni_get_head(moni_shmaddr);
        shm_lock = &head->lock;
    } 
    if (!hash_map_lock && lock != shm_lock) {
        hash_map_lock = lock;
    }
    if (lock == hash_map_lock) {
        spins = &hash_map_lock_spins;
        calls = &hash_map_lock_calls;
    }
    if (lock == shm_lock) {
        spins = &shm_lock_spins;
        calls = &shm_lock_calls;
    }
    assert(spins && calls);
    moni_atomic_add(spins, 1);
    moni_atomic_add(calls, 1);
#endif
    while (__sync_lock_test_and_set(lock, LOCK)) {             
#ifndef NDEBUG
    moni_atomic_add(spins, 1);
#endif
        sched_yield();                                          
        // 设置最大尝试次数，防止死锁
        // 当共享内存中的lock置1后，如果进程异常退出，lock不会被置1，这种情况下会导致死锁
        if (cnt++ > 10000) {
            fprintf(stderr, "warn: dead lock may occur, lock: %p.\n", lock);
            break;
        }
    }                                                           
#ifndef NDEBUG
    if (lock == hash_map_lock && cnt > hash_map_lock_max_spins) hash_map_lock_max_spins = cnt;
    if (lock == shm_lock && cnt > shm_lock_max_spins) shm_lock_max_spins = cnt;
#endif
} 

// release
void moni_spin_unlock(moni_spin_lock_t* lock) {                             
    __sync_lock_release(lock); 
}
