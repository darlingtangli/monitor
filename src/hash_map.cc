#include "hash_map.h"

#include <string.h>
#include "hash.h"
#include "shm_data.h"

namespace inv 
{

namespace monitor
{

HashMap::HashMap(void* addr) 
    : _shm_data_addr(addr)
{
    memset(&_table[0], RAW, sizeof(_table));
}

int HashMap::Get(const std::string& key)
{
    uint32_t cnt = 0;
    int i = BKDRHash(key.c_str()) % BUCKET_NUM;
    while (_table[i] != RAW)
    {
        Entry* entry = GetEntry(_shm_data_addr, _table[i]);
        if (!strncmp((char*)entry->data.metric, key.c_str(), sizeof(entry->data.metric)-1)) return _table[i];
        i = (i + 1) % BUCKET_NUM; // 线性探测解决HASH冲突
        if (++cnt > BUCKET_NUM) return -1; // 死循环预防
    }
    return -1;
}

int HashMap::Set(const std::string& key, uint32_t value)
{
    uint32_t cnt = 0;
    int i = BKDRHash(key.c_str()) % BUCKET_NUM;
    while (_table[i] != RAW)
    {
        i = (i + 1) % BUCKET_NUM; // 线性探测解决HASH冲突
        if (++cnt > BUCKET_NUM) return -1; // 死循环预防
    }
    _table[i] = value;
    return i;
}

} // namespace monitor

} // namespace inv
