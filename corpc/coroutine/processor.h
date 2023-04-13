/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_PROCESSOR_H
#define CORPC_COROUTINE_PROCESSOR_H
#include <queue>
#include <set>
#include <mutex>
#include <thread>
#include <memory>
#include <map>
#include <vector>
#include "objpool.h"
#include "spinlock.h"
#include "context.h"
#include "coroutine.h"
#include "epoller.h"
#include "timer.h"
#include "lock_free_ring_buffer.h"
#include "fd_event.h"
#include "../log/logger.h"

namespace corpc
{
	extern __thread int threadIdx; // each thread has one

	enum processerStatus
	{
		PRO_RUNNING = 0,
		PRO_STOPPING,
		PRO_STOPPED
	};

	class Processor
	{
	public:
		using ptr = std::shared_ptr<Processor>;

		explicit Processor(int);

		~Processor();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Processor);

		// wait event on fd
		void waitEvent(FdEvent::ptr fd_event, int fd, int event);

		Coroutine *getNewCoroutine(std::function<void()> &&func, size_t stackSize = parameter::coroutineStackSize);

		Coroutine *getNewCoroutine(std::function<void()> &func, size_t stackSize = parameter::coroutineStackSize);

		void addCoroutine(corpc::Coroutine *cor, bool is_wakeup = true);

		// Resume the specified coroutine
		void resume(Coroutine *);

		void yield();

		// Current coroutine waiting interval(milliseconds)
		void wait(int64_t interval);

		// Add the currently running coroutine to the delete list
		void killCurCo();

		bool loop();

		void stop();

		void join();

	public:
		// Obtain the currently running coroutine
		inline Coroutine *getCurRunningCo() { return m_cur_coroutine; };

		inline Context *getMainCtx() { return &m_main_ctx; }

		inline size_t getCoCnt() { return m_cors_set.size(); }

		inline Epoller *getEpoller()
		{
			return m_epoller.get();
		}

		inline Timer *getTimer()
		{
			return m_timer.get();
		}

	private:
		bool isLoopThread() const;

	private:
		// The number of the processor
		int m_tid;
		int m_status;

		Epoller::ptr m_epoller;
		Timer::ptr m_timer;

		std::mutex m_mutex;
		std::thread *m_pLoop;

		// new coming tasks
		LockFreeRingBuffer<Coroutine *, 1000> m_pending_tasks;

		// List of active events discovered by EventEpoller
		std::vector<Coroutine *> m_active_tasks;

		// The collection of all unfinished coroutines on this thread
		std::set<Coroutine *> m_cors_set;

		// The list of removed coroutines will first be placed in the list to remove an event, and only after one loop is completed will it be truly deleted
		std::vector<Coroutine *> m_removed_cors;

		// Lock for objPool
		Spinlock m_cor_pool_lock;

		// objPool
		ObjPool<Coroutine> m_cor_pool;

		Coroutine *m_cur_coroutine;

		Context m_main_ctx;
	};

}

#endif
