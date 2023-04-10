/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_SPINLOCK_H
#define CORPC_COROUTINE_SPINLOCK_H
#include <atomic>
#include "utils.h"

namespace corpc
{

	// 配合std::atomic_int定义的二元信号量使用，为1表示资源可以使用，为0表示资源不可使用
	class Spinlock
	{
	public:
		Spinlock()
			: m_sem(1)
		{
		}

		~Spinlock() { unlock(); }

		DISALLOW_COPY_MOVE_AND_ASSIGN(Spinlock);

		void lock()
		{
			int exp = 1;
			// 当m_sem的值和exp值相等时，m_sem被赋值为0,返回true，所以不进入循环
			while (!m_sem.compare_exchange_strong(exp, 0))
			{
				exp = 1;
			}
		}

		void unlock()
		{
			m_sem.store(1);
		}

	private:
		std::atomic_int m_sem;
	};

}

#endif