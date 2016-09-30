#include "hash.h"

namespace inv 
{

namespace monitor
{

// 和murmur hash(https://en.wikipedia.org/wiki/MurmurHash)用真实路径实测比较，
// 冲突率都在1/40000左右，但bkdr hash实现更简单
uint32_t BKDRHash(const char *str)
{
    unsigned int seed = 13131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return hash;
}

} // namespace monitor

} // namespace inv
