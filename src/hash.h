/**
 * @file hash.h
 * @brief 字符串hash成整数
 * @author litang
 * @version 1.0
 * @date 2016-05-30
 */
#ifndef __HASH_H
#define __HASH_H

#include <stdint.h>

namespace inv 
{

namespace monitor
{

uint32_t BKDRHash(const char* str);

} // namespace monitor

} // namespace inv

#endif // __HASH_H

