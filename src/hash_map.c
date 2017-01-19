#include "hash_map.h"

#include <string.h>
#include "hash.h"
#include "shm_data.h"

#ifndef NDEBUG
#include "atomic.h"
uint64_t hash_map_set_conflics = 0;
uint64_t hash_map_set_count    = 0;
uint64_t hash_map_get_conflics = 0;
uint64_t hash_map_get_count    = 0;
#endif

void moni_hash_map_init(moni_hash_map_t* hmap, moni_hash_check_t check_cb) {
    memset(&hmap->bucket[0], RAW, sizeof(hmap->bucket));
    hmap->check_cb = check_cb;
    return;
}

int moni_hash_map_get(moni_hash_map_t* hmap, const char* key) {
    uint32_t cnt;
    uint32_t hash;
    uint32_t i;

#ifndef NDEBUG
    moni_atomic_add(&hash_map_get_count, 1);
#endif
    moni_bkdr_hash(key, &hash);
    i = hash  % BUCKET_NUM;
    cnt = 0;
    while (hmap->bucket[i] != RAW) {
#ifndef NDEBUG
        moni_atomic_add(&hash_map_get_conflics, 1);
#endif
        if (hmap->check_cb(key, hmap->bucket[i])) return hmap->bucket[i];
        i = (i + 1) % BUCKET_NUM; // 线性探测解决HASH冲突
        if (++cnt > BUCKET_NUM) return -1; // 死循环预防
    }
    return -1;
}

int moni_hash_map_set(moni_hash_map_t* hmap, const char* key, uint32_t value) {
    uint32_t cnt;
    uint32_t hash;
    uint32_t i;

#ifndef NDEBUG
    moni_atomic_add(&hash_map_set_count, 1);
#endif
    moni_bkdr_hash(key, &hash);
    i = hash % BUCKET_NUM;
    cnt = 0;
    while (hmap->bucket[i] != RAW) {
#ifndef NDEBUG
        moni_atomic_add(&hash_map_set_conflics, 1);
#endif
        if (hmap->check_cb(key, hmap->bucket[i])) return i;
        i = (i + 1) % BUCKET_NUM; // 线性探测解决HASH冲突
        if (++cnt > BUCKET_NUM) return -1; // 死循环预防
    }
    hmap->bucket[i] = value;
    return i;
}

