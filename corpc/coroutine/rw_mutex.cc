/***
    @author: Wangzhiming
    @date: 2022-10-29
***/
#include "rw_mutex.h"
#include "scheduler.h"
#include "spinlock_guard.h"

using namespace corpc;

void RWMutex::rlock()
{

    {
        SpinlockGuard l(lock_);
        if (state_ == MU_FREE || state_ == MU_READING)
        {
            readingNum_.fetch_add(1);
            state_ = MU_READING;
            return;
        }

        waitingCo_.push(Scheduler::getScheduler()->getProcessor(threadIdx)->getCurRunningCo());
    }
    LogInfo("in rlock, yield to wait RWMutex");
    Scheduler::getScheduler()->getProcessor(threadIdx)->yield();
    rlock();
}

void RWMutex::runlock()
{
    SpinlockGuard l(lock_);
    auto cur = readingNum_.fetch_add(-1);
    if (cur == 1)
    {
        freeLock();
    }
}

void RWMutex::wlock()
{

    {
        SpinlockGuard l(lock_);
        if (state_ == MU_FREE)
        {
            state_ = MU_WRITING;
            return;
        }
        LogInfo("in wlock, yield to wait RWMutex");
        waitingCo_.push(Scheduler::getScheduler()->getProcessor(threadIdx)->getCurRunningCo());
    }

    Scheduler::getScheduler()->getProcessor(threadIdx)->yield();
    wlock();
}

void RWMutex::wunlock()
{
    SpinlockGuard l(lock_);
    freeLock();
}

void RWMutex::freeLock()
{
    state_ = MU_FREE;
    while (!waitingCo_.empty())
    {
        auto wakeCo = waitingCo_.front();
        waitingCo_.pop();
        LogInfo("in freeLock, go to wake RWMutex");
        wakeCo->getMyProcessor()->goCo(wakeCo);
    }
}