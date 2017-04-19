/**
 * @file spin_lock.h
 * @brief 用户态自旋锁简单实现
 * @author darlingtangli@gmail.com
 * @version 1.0
 * @date 2016-05-12
 */
#ifndef __SPIN_LOCK_H
#define __SPIN_LOCK_H

#include <stdint.h>

typedef uint8_t moni_spin_lock_t;
#define UNLOCK  0  // must be 0
#define LOCK    1

void moni_spin_lock(moni_spin_lock_t* lock);
void moni_spin_unlock(moni_spin_lock_t* lock);

#endif //  __SPIN_LOCK_H
