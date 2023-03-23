/***
    @author: Wangzhiming
    @date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_MUTEX_H
#define CORPC_COROUTINE_MUTEX_H
#include "coroutine.h"
#include "spinlock.h"

#include <atomic>
#include <queue>

namespace corpc
{

    enum muStatus
    {
        MU_FREE = 0,
        MU_READING,
        MU_WRITING
    };

    // 读写锁
    class RWMutex
    {
    public:
        RWMutex()
            : state_(MU_FREE), readingNum_(0){};
        ~RWMutex(){};

        DISALLOW_COPY_MOVE_AND_ASSIGN(RWMutex);

        // 读锁
        void rlock();
        // 解读锁
        void runlock();

        // 写锁
        void wlock();
        // 解写锁
        void wunlock();

    private:
        void freeLock();

        int state_;

        std::atomic_int readingNum_;

        Spinlock lock_;

        std::queue<Coroutine *> waitingCo_;
    };

}

#endif