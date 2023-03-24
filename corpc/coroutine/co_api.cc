/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "co_api.h"
#include "../log/logger.h"

void corpc::co_go(std::function<void()> &&func, size_t stackSize, int tid)
{
	// if (tid < 0)
	// {
	// 	// LogTrace("随机选择processor");
	// 	corpc::Scheduler::getScheduler()->createNewCo(std::move(func), stackSize);
	// }
	// else
	// {
	// 	// LogTrace("指定processor为" << tid);
	// 	tid %= corpc::Scheduler::getScheduler()->getProCnt();
	// 	corpc::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(std::move(func), stackSize);
	// }
}

void corpc::co_go(std::function<void()> &func, size_t stackSize, int tid)
{
	// if (tid < 0)
	// {
	// 	corpc::Scheduler::getScheduler()->createNewCo(func, stackSize);
	// }
	// else
	// {
	// 	tid %= corpc::Scheduler::getScheduler()->getProCnt();
	// 	corpc::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(func, stackSize);
	// }
}

void corpc::co_sleep(Time time)
{
	// corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);
}

void corpc::sche_join()
{
	corpc::Scheduler::getScheduler()->join();
}