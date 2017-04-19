/**
 * @file hash_map.h
 * @brief hash map简单实现，用于提高查询metric在共享内存中的下标时的效率
 * @author darlingtangli@gmail.com
 * @version 1.0
 * @date 2016-06-13
 */
#ifndef __HASH_MAP_H
#define __HASH_MAP_H

#include <stdint.h>

/* NOTE: 确保BUCKET_NUM大于共享内存最大能支持的指标数，否则会导致冲突率升高而降低性能 */
#define BUCKET_NUM 249989
#define RAW 0xFFFFFFFF
#define MAX_KEY_LEN 64

typedef const char* moni_hash_key_t;
typedef uint32_t moni_hash_value_t;
typedef int (*moni_hash_check_t)(moni_hash_key_t, moni_hash_value_t);

typedef struct moni_hast_map_s {
    moni_hash_value_t bucket[BUCKET_NUM];
    moni_hash_check_t check_cb;
} moni_hash_map_t;

void moni_hash_map_init(moni_hash_map_t* hmap, moni_hash_check_t check_cb);
int  moni_hash_map_get (moni_hash_map_t* hmap, moni_hash_key_t key);
int  moni_hash_map_set (moni_hash_map_t* hmap, moni_hash_key_t key, moni_hash_value_t value);

#endif // __HASH_MAP_H
