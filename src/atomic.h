/**
 * @file atomic.h
 * @brief 原子操作函数
 * @author litang
 * @version 1.0
 * @date 2016-05-17
 */
#ifndef __ATOMIC_H
#define __ATOMIC_H

namespace inv 
{

namespace monitor
{

struct Atomic
{

template <typename T>
static T Add(T* ptr, T value)
{
    return __sync_add_and_fetch(ptr, value);
}

};

} // namespace monitor

} // namespace inv

#endif // __ATOMIC_H
