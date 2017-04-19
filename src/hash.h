/**
 * @file hash.h
 * @brief 字符串hash成整数
 * @author darlingtangli@gmail.com
 * @version 1.0
 * @date 2016-05-30
 */
#ifndef __HASH_H
#define __HASH_H

/**
 * 和murmur hash(https://en.wikipedia.org/wiki/MurmurHash)用真实路径实测比较，
 * 冲突率都在1/40000左右，但bkdr hash实现更简单
 */
#define moni_bkdr_hash(key, value) do {                             \
    unsigned int seed = 13131; /*31 131 1313 13131 131313 etc.. */  \
    const char* ptr = key;                                          \
    *(value) = 0;                                                   \
    while (*ptr) {                                                  \
        *(value) = *(value) * seed + *ptr++;                        \
    }                                                               \
} while (0)

#endif // __HASH_H

