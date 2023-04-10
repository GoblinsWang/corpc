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

		void waitEvent(FdEvent::ptr fd_event, int fd, int event);

		void addTask(std::function<void()> task, bool is_wakeup = true);

		void addTask(std::vector<std::function<void()>> task, bool is_wakeup = true);

		Coroutine *getNewCoroutine(std::function<void()> &&func, size_t stackSize = parameter::coroutineStackSize);

		Coroutine *getNewCoroutine(std::function<void()> &func, size_t stackSize = parameter::coroutineStackSize);

		void addCoroutine(corpc::Coroutine *cor, bool is_wakeup = true);

		void Resume();

		// 恢复运行指定协程
		void resume(Coroutine *);

		void yield();

		// 当前协程等待time毫秒
		void wait(int64_t interval);

		// 清除当前正在运行的协程
		void killCurCo();

		bool loop();

		void stop();

		void join();

		// 获取当前正在运行的协程
		inline Coroutine *getCurRunningCo() { return m_pCurCoroutine; };

		inline Context *getMainCtx() { return &m_mainCtx; }

		inline size_t getCoCnt() { return m_coSet.size(); }

		void goCo(Coroutine *co);

		void goCoBatch(std::vector<Coroutine *> &cos);

	public:
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
		// 该处理器的线程号
		int m_tid;

		int m_status;

		Epoller::ptr m_epoller;

		Timer::ptr m_timer;

		std::mutex m_mutex;
		std::thread *m_pLoop;

		std::map<int, epoll_event> m_pending_add_fds;
		std::vector<int> m_pending_del_fds;
		std::vector<std::function<void()>> m_pending_tasks;

		LockFreeRingBuffer<Coroutine *, 1000> m_newCoroutines;

		// 对象池的锁
		Spinlock m_coPoolLock;

		// EventEpoller发现的活跃事件所放的列表
		std::vector<Coroutine *> m_actCoroutines;

		std::set<Coroutine *> m_coSet;

		// 被移除的协程列表，要移除某一个事件会先放在该列表中，一次循环结束才会真正delete
		std::vector<Coroutine *> m_removedCo;

		// 对象池
		ObjPool<Coroutine> m_copool;

		Coroutine *m_pCurCoroutine;

		Context m_mainCtx;
	};

}

#endif
