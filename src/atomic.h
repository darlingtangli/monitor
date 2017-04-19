/**
 * @file atomic.h
 * @brief 原子操作函数
 * @author darlingtangli@gmail.com
 * @version 1.0
 * @date 2016-05-17
 */
#ifndef __ATOMIC_H
#define __ATOMIC_H

#define moni_atomic_add(ptr, value) __sync_add_and_fetch(ptr, value)

#endif // __ATOMIC_H
