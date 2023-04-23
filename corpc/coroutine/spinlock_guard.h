/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_SPINm_lockGUARD_H
#define CORPC_COROUTINE_SPINm_lockGUARD_H
#include "spinlock.h"
#include "utils.h"

namespace corpc
{

	// 配合std::atomic_int定义的二元信号量使用，为1表示资源可以使用，为0表示资源不可使用
	class SpinlockGuard
	{
	public:
		SpinlockGuard(Spinlock &l)
			: m_lock(l)
		{
			m_lock.lock();
		}

		~SpinlockGuard()
		{
			m_lock.unlock();
		}

		DISALLOW_COPY_MOVE_AND_ASSIGN(SpinlockGuard);

	private:
		Spinlock &m_lock;
	};

}
#endif