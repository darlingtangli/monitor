/**
 * @file hash_map.h
 * @brief hash map简单实现，用于提高查询metric在共享内存中的下标时的效率
 * @author litang
 * @version 1.0
 * @date 2016-06-13
 */
#ifndef __HASH_MAP_H
#define __HASH_MAP_H

#include <stdint.h>
#include <string>

namespace inv 
{

namespace monitor
{

class HashMap
{
public:
    // NOTE: 确保BUCKET_NUM大于共享内存最大能支持的指标数，否则会导致冲突率升高而降低性能
    static const uint32_t BUCKET_NUM = 249989; 
    static const uint32_t RAW = -1;

public:
    HashMap(void* addr);

    int Get(const std::string& key);
    int Set(const std::string& key, uint32_t value);

private:
    uint32_t _table[BUCKET_NUM];
    void*    _shm_data_addr;

};

} // namespace monitor

} // namespace inv

#endif // __HASH_MAP_H
