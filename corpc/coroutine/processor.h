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
#include "../log/logger.h"

namespace corpc
{
	extern __thread int threadIdx; // 每个线程都会有一份

	enum processerStatus
	{
		PRO_RUNNING = 0,
		PRO_STOPPING,
		PRO_STOPPED
	};

	enum newCoAddingStatus
	{
		NEWCO_ADDING = 0,
		NEWCO_ADDED
	};

	class Processor
	{
	public:
		using ptr = std::shared_ptr<Processor>;

		explicit Processor(int);

		~Processor();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Processor);

		void waitEvent(int fd, int event);

		void addEvent(int fd, epoll_event event, bool is_wakeup = true);

		void delEvent(int fd, bool is_wakeup = true);

		void addTask(std::function<void()> task, bool is_wakeup = true);

		void addTask(std::vector<std::function<void()>> task, bool is_wakeup = true);

		void addCoroutine(corpc::Coroutine *cor, bool is_wakeup = true);

		void yield();

		// 当前协程等待time毫秒
		void wait(Time time);

		// 清除当前正在运行的协程
		void killCurCo();

		bool loop();

		void stop();

		void join();

		// 获取当前正在运行的协程
		inline Coroutine *getCurRunningCo() { return pCurCoroutine_; };

		inline Context *getMainCtx() { return &mainCtx_; }

		inline size_t getCoCnt() { return coSet_.size(); }

		void goCo(Coroutine *co);

		void goCoBatch(std::vector<Coroutine *> &cos);

	public:
		static Processor *GetProcessor();

		void addEventInLoopThread(int fd, epoll_event event);

		void delEventInLoopThread(int fd);

		// 恢复运行指定协程
		void resume(Coroutine *);

	private:
		bool isLoopThread() const;

	private:
		// 该处理器的线程号
		int m_tid;

		int m_status;

		Epoller::ptr m_epoller;

		Timer::ptr m_timer;

		std::mutex m_mutex;
		std::thread *pLoop_;

		std::map<int, epoll_event> m_pending_add_fds;
		std::vector<int> m_pending_del_fds;
		std::vector<std::function<void()>> m_pending_tasks;

		// 新任务队列，使用双缓存队列
		std::queue<Coroutine *> newCoroutines_[2];

		// 新任务双缓存队列中正在运行的队列号，另一条用于添加任务
		volatile int runningNewQue_;

		Spinlock newQueLock_;

		Spinlock coPoolLock_;

		// std::mutex newCoQueMtx_;

		// EventEpoller发现的活跃事件所放的列表
		std::vector<Coroutine *> m_actCoroutines;

		std::set<Coroutine *> coSet_;

		// 定时器任务列表
		std::vector<Coroutine *> timerExpiredCo_;

		// 被移除的协程列表，要移除某一个事件会先放在该列表中，一次循环结束才会真正delete
		std::vector<Coroutine *> removedCo_;

		// 对象池
		ObjPool<Coroutine> m_copool;

		Coroutine *pCurCoroutine_;

		Context mainCtx_;
	};

}

#endif
