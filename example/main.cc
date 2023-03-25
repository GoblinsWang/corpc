/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <iostream>
#include <thread>
#include <sys/sysinfo.h>
#include "../corpc/coroutine/scheduler.h"
#include "../corpc/coroutine/processor.h"
#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/rw_mutex.h"
#include "../corpc/log/logger.h"

void testCoroutine()
{
	corpc::Scheduler *Sc = corpc::Scheduler::getScheduler();
	for (int i = 0; i < 50; ++i)
	{
		auto func = []()
		{ std::cout << "hello world" << endl; };

		corpc::Coroutine *cor = Sc->getNewCoroutine(func);
		Sc->getProcessor()->addCoroutine(cor);
	}
}

int main()
{
	testCoroutine();
	corpc::sche_join();

	return 0;
}
