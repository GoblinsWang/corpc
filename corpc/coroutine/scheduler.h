/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_SCHEDULER_H
#define CORPC_COROUTINE_SCHEDULER_H
#include <vector>
#include <functional>
#include "coroutine.h"
#include "spinlock.h"
#include "processor.h"
#include "processor_selector.h"
#include "parameter.h"

namespace corpc
{

	class Scheduler
	{
	protected:
		Scheduler();
		~Scheduler();

	public:
		DISALLOW_COPY_MOVE_AND_ASSIGN(Scheduler);

		static Scheduler *getScheduler();

		Coroutine *getNewCoroutine(std::function<void()> &&func, size_t stackSize = parameter::coroutineStackSize);

		Coroutine *getNewCoroutine(std::function<void()> &func, size_t stackSize = parameter::coroutineStackSize);

		// 指定实例
		Processor *getProcessor(int id);

		// 不指定实例，按照策略进行选择
		Processor *getProcessor();

		int getProCnt();

		void join();

	private:
		// 初始化Scheduler，threadCnt为开启几个线程
		bool startScheduler(int threadCnt);

		// 日志管理器实例
		static Scheduler *m_pScheduler;

		// 用于保护的锁，为了服务器执行效率，原则上不允许长久占有此锁
		static std::mutex m_scherMtx;

		std::vector<Processor *> m_processors;

		ProcessorSelector m_proSelector;

	private:
		Spinlock m_coPoolLock;

		// 对象池
		ObjPool<Coroutine> m_copool;
	};

}
#endif