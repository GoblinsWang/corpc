/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "co_api.h"
#include "../log/logger.h"

void corpc::co_go(std::function<void()> &&func, int tid, size_t stackSize)
{
	if (tid < 0)
	{
		corpc::Processor *pro = corpc::Scheduler::getScheduler()->getProcessor();
		corpc::Coroutine *cor = pro->getNewCoroutine(std::move(func));
		pro->addCoroutine(cor);
	}
	else
	{
		corpc::Processor *pro = corpc::Scheduler::getScheduler()->getProcessor(tid); // 指定
		corpc::Coroutine *cor = pro->getNewCoroutine(std::move(func));
		pro->addCoroutine(cor);
	}
}

void corpc::co_go(std::function<void()> &func, int tid, size_t stackSize)
{
	if (tid < 0)
	{
		corpc::Processor *pro = corpc::Scheduler::getScheduler()->getProcessor();
		corpc::Coroutine *cor = pro->getNewCoroutine(func);
		pro->addCoroutine(cor);
	}
	else
	{
		corpc::Processor *pro = corpc::Scheduler::getScheduler()->getProcessor(tid); // 指定
		corpc::Coroutine *cor = pro->getNewCoroutine(func);
		pro->addCoroutine(cor);
	}
}

void corpc::co_sleep(int64_t interval)
{
	corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(interval);
}

void corpc::sche_join()
{
	corpc::Scheduler::getScheduler()->join();
}