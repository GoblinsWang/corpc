/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "co_api.h"
#include "../log/logger.h"

void corpc::co_go(std::function<void()> &&func, size_t stackSize, int tid)
{
	corpc::Scheduler *Sc = corpc::Scheduler::getScheduler();
	corpc::Coroutine *cor = Sc->getNewCoroutine(std::move(func), stackSize);
	if (tid < 0)
	{
		Sc->getProcessor()->addCoroutine(cor);
	}
	else
	{
		Sc->getProcessor(tid)->addCoroutine(cor);
	}
}

void corpc::co_go(std::function<void()> &func, size_t stackSize, int tid)
{
	corpc::Scheduler *Sc = corpc::Scheduler::getScheduler();
	corpc::Coroutine *cor = Sc->getNewCoroutine(func, stackSize);
	if (tid < 0)
	{
		Sc->getProcessor()->addCoroutine(cor);
	}
	else
	{
		Sc->getProcessor(tid)->addCoroutine(cor);
	}
}

void corpc::co_sleep(Time time)
{
	corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);
}

void corpc::sche_join()
{
	corpc::Scheduler::getScheduler()->join();
}