/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "scheduler.h"
#include "spinlock_guard.h"
#include <sys/sysinfo.h>

using namespace corpc;

Scheduler *Scheduler::m_pScheduler = nullptr;
std::mutex Scheduler::m_scherMtx;

Scheduler::Scheduler()
	: m_proSelector(m_processors)
{
}

Scheduler::~Scheduler()
{
	for (auto pP : m_processors)
	{
		pP->stop();
	}
	for (auto pP : m_processors)
	{
		pP->join();
		delete pP;
	}
}

bool Scheduler::startScheduler(int threadCnt)
{
	for (int i = 0; i < threadCnt; ++i)
	{
		LogDebug("init " << i << " processor");
		m_processors.emplace_back(new Processor(i));
		m_processors[i]->loop();
	}
	return true;
}

Scheduler *Scheduler::getScheduler()
{
	if (nullptr == m_pScheduler)
	{
		std::lock_guard<std::mutex> lock(m_scherMtx);
		if (nullptr == m_pScheduler)
		{
			m_pScheduler = new Scheduler();
			m_pScheduler->startScheduler(::get_nprocs_conf());
		}
	}
	return m_pScheduler;
}

void Scheduler::join()
{
	for (auto pP : m_processors)
	{
		pP->join();
	}
}

Processor *Scheduler::getProcessor(int id)
{
	return m_processors[id];
}

Processor *Scheduler::getProcessor()
{
	return m_proSelector.next();
}

int Scheduler::getProCnt()
{
	return static_cast<int>(m_processors.size());
}