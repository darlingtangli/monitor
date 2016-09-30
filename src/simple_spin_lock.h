/**
 * @file simple_spin_lock.h
 * @brief 用户态自旋锁简单实现
 * @author litang
 * @version 1.0
 * @date 2016-05-12
 */
#ifndef __SIMPLE_SPIN_LOCK_H
#define __SIMPLE_SPIN_LOCK_H

#include <sched.h>

namespace inv 
{

namespace monitor
{

struct SimpleSpinLock
{
    typedef uint8_t LockState;
    const static LockState UNLOCK = 0;
    const static LockState LOCK   = 1;

    static void Lock(LockState* ls)
    {
        int cnt = 0;
        while (!__sync_bool_compare_and_swap(ls, UNLOCK, LOCK))
        {
            sched_yield();
            // 循环多次后还不能获得锁，可能发生了死锁，强制释放锁
            if (cnt++>1000) *ls = UNLOCK;
        }
    }

    static void UnLock(LockState* ls)
    {
        *ls = UNLOCK;
    }
};

class LockGuard
{
public:
    LockGuard(SimpleSpinLock::LockState* ls) : _status(ls) {SimpleSpinLock::Lock(_status);}
    ~LockGuard() {SimpleSpinLock::UnLock(_status);}

private:
    SimpleSpinLock::LockState* _status;
};

} // namespace monitor

} // namespace inv

#endif //  __SIMPLE_SPIN_LOCK_H
