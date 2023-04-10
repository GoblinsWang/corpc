/***
    @author: Wangzhiming
    @date: 2022-10-29
***/
#include "rw_mutex.h"
#include "scheduler.h"
#include "spinlock_guard.h"

namespace corpc
{

    void RWMutex::rlock()
    {

        {
            SpinlockGuard l(m_lock);
            if (m_state == MU_FREE || m_state == MU_READING)
            {
                m_readingNum.fetch_add(1);
                m_state = MU_READING;
                return;
            }

            m_waitingCo.push(Scheduler::getScheduler()->getProcessor(threadIdx)->getCurRunningCo());
        }
        // LogInfo("in rlock, yield to wait RWMutex");
        Scheduler::getScheduler()->getProcessor(threadIdx)->yield();
        rlock();
    }

    void RWMutex::runlock()
    {
        SpinlockGuard l(m_lock);
        auto cur = m_readingNum.fetch_add(-1);
        if (cur == 1)
        {
            freeLock();
        }
    }

    void RWMutex::wlock()
    {

        {
            SpinlockGuard l(m_lock);
            if (m_state == MU_FREE)
            {
                m_state = MU_WRITING;
                return;
            }
            // LogInfo("in wlock, yield to wait RWMutex");
            m_waitingCo.push(Scheduler::getScheduler()->getProcessor(threadIdx)->getCurRunningCo());
        }

        Scheduler::getScheduler()->getProcessor(threadIdx)->yield();
        wlock();
    }

    void RWMutex::wunlock()
    {
        SpinlockGuard l(m_lock);
        freeLock();
    }

    void RWMutex::freeLock()
    {
        m_state = MU_FREE;
        while (!m_waitingCo.empty())
        {
            auto wakeCo = m_waitingCo.front();
            m_waitingCo.pop();
            // LogInfo("in freeLock, go to wake RWMutex");
            wakeCo->getMyProcessor()->addCoroutine(wakeCo);
        }
    }
}