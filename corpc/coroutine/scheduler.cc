/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "scheduler.h"
#include "spinlock_guard.h"
#include <sys/sysinfo.h>

using namespace corpc;

Scheduler *Scheduler::pScher_ = nullptr;
std::mutex Scheduler::scherMtx_;

Scheduler::Scheduler()
	: proSelector_(processors_)
{
}

Scheduler::~Scheduler()
{
	for (auto pP : processors_)
	{
		pP->stop();
	}
	for (auto pP : processors_)
	{
		pP->join();
		delete pP;
	}
}

bool Scheduler::startScheduler(int threadCnt)
{
	for (int i = 0; i < threadCnt; ++i)
	{
		LogDebug("init " << i << "processor");
		processors_.emplace_back(new Processor(i));
		processors_[i]->loop();
	}
	return true;
}

Scheduler *Scheduler::getScheduler()
{
	if (nullptr == pScher_)
	{
		std::lock_guard<std::mutex> lock(scherMtx_);
		if (nullptr == pScher_)
		{
			pScher_ = new Scheduler();
			pScher_->startScheduler(::get_nprocs_conf());
		}
	}
	return pScher_;
}

Coroutine *Scheduler::getNewCoroutine(std::function<void()> &&coFunc, size_t stackSize)
{
	Coroutine *pCo = nullptr;

	{
		SpinlockGuard lock(m_coPoolLock);
		pCo = m_copool.new_obj(std::move(coFunc), stackSize);
	}
	return pCo;
}
Coroutine *Scheduler::getNewCoroutine(std::function<void()> &coFunc, size_t stackSize)
{
	Coroutine *pCo = nullptr;

	{
		SpinlockGuard lock(m_coPoolLock);
		pCo = m_copool.new_obj(coFunc, stackSize);
	}
	return pCo;
}

void Scheduler::join()
{
	for (auto pP : processors_)
	{
		pP->join();
	}
}

Processor *Scheduler::getProcessor(int id)
{
	return processors_[id];
}

Processor *Scheduler::getProcessor()
{
	return proSelector_.next();
}

int Scheduler::getProCnt()
{
	return static_cast<int>(processors_.size());
}