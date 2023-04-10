/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "scheduler.h"
#include "spinlock_guard.h"
#include <sys/sysinfo.h>

namespace corpc
{

	Scheduler *Scheduler::m_scheduler = nullptr;
	std::mutex Scheduler::m_sche_mutex;

	Scheduler::Scheduler()
		: m_pro_cnt(::get_nprocs_conf()), m_selector(m_processors)
	{
	}

	Scheduler::~Scheduler()
	{
		for (auto pro : m_processors)
		{
			pro->stop();
		}
		for (auto pro : m_processors)
		{
			pro->join();
			delete pro;
		}
	}

	Scheduler *Scheduler::getScheduler()
	{
		if (nullptr == m_scheduler)
		{
			std::lock_guard<std::mutex> lock(m_sche_mutex);
			if (nullptr == m_scheduler)
			{
				m_scheduler = new Scheduler();
				m_scheduler->startScheduler(m_scheduler->getProCnt());
			}
		}
		return m_scheduler;
	}

	Processor *Scheduler::getProcessor(int id)
	{
		return m_processors[id];
	}

	Processor *Scheduler::getProcessor()
	{
		return m_selector.next();
	}

	void Scheduler::join()
	{
		for (auto pro : m_processors)
		{
			pro->join();
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
}