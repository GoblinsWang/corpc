/***
    @author: Wangzhiming
    @date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_MUTEX_H
#define CORPC_COROUTINE_MUTEX_H
#include "coroutine.h"
#include "spinlock.h"
#include "../log/logger.h"

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
        : m_state(MU_FREE), m_readingNum(0){};
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

    int m_state;

    std::atomic_int m_readingNum;

    Spinlock m_lock;

    std::queue<Coroutine *> m_waitingCo;
  };

}

#endif