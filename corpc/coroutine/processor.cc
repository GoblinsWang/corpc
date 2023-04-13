/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <assert.h>
#include "processor.h"
#include "parameter.h"
#include "spinlock_guard.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <algorithm>
#include <functional>

namespace corpc
{
	__thread int threadIdx = -1;

	Processor::Processor(int tid)
		: m_tid(tid), m_status(PRO_STOPPED), m_pLoop(nullptr), m_cur_coroutine(nullptr), m_main_ctx(0)
	{
		m_main_ctx.makeCurContext();
	}

	Processor::~Processor()
	{
		if (PRO_RUNNING == m_status)
		{
			stop();
		}
		if (PRO_STOPPING == m_status)
		{
			join();
		}
		if (nullptr != m_pLoop)
		{
			delete m_pLoop;
		}
		for (auto co : m_cors_set)
		{
			delete co;
		}
	}

	void Processor::waitEvent(FdEvent::ptr fd_event, int fd, int event)
	{
		// LogDebug("in waitEvent, fd = " << fd);

		fd_event->setCoroutine(m_cur_coroutine);
		m_epoller->addEvent(fd_event.get(), fd, event);

		yield();

		m_epoller->delEvent(fd);
		fd_event->clearCoroutine();
	}

	Coroutine *Processor::getNewCoroutine(std::function<void()> &&coFunc, size_t stackSize)
	{
		Coroutine *pCo = nullptr;

		{
			SpinlockGuard lock(m_cor_pool_lock);
			pCo = m_cor_pool.new_obj(std::move(coFunc), stackSize);
		}
		return pCo;
	}
	Coroutine *Processor::getNewCoroutine(std::function<void()> &coFunc, size_t stackSize)
	{
		Coroutine *pCo = nullptr;

		{
			SpinlockGuard lock(m_cor_pool_lock);
			pCo = m_cor_pool.new_obj(coFunc, stackSize);
		}
		return pCo;
	}

	void Processor::addCoroutine(corpc::Coroutine *pCo, bool is_wakeup /*=true*/)
	{
		pCo->setProcessor(this);
		m_pending_tasks.push(pCo);

		if (is_wakeup)
		{
			LogDebug("in processor " << m_tid << ", to wakeup epoller");
			m_epoller->wakeup();
		}
	}

	void Processor::resume(Coroutine *pCo)
	{
		if (nullptr == pCo)
		{
			return;
		}

		if (m_cors_set.find(pCo) == m_cors_set.end())
		{
			return;
		}
		// LogInfo("in Processor::resume()");
		m_cur_coroutine = pCo;
		pCo->resume();
	}

	void Processor::yield()
	{
		m_cur_coroutine->yield();
		/*
			Switch to the main thread and then save the context of the current coroutine into the object of the current coroutine.
		*/
		m_main_ctx.swapToMe(m_cur_coroutine->getCtx());
	}

	void Processor::wait(int64_t interval)
	{
		m_cur_coroutine->yield();
		Coroutine *cor = m_cur_coroutine;
		auto event_cb = [this, cor]()
		{
			this->resume(cor);
		};
		TimerEvent::ptr event = std::make_shared<TimerEvent>(interval, false, event_cb);
		m_timer->addTimerEvent(event);
		/*
			Switch to the main thread and then save the context of the current coroutine into the object of the current coroutine.
		*/
		m_main_ctx.swapToMe(m_cur_coroutine->getCtx());
	}

	bool Processor::loop()
	{
		m_epoller = std::make_shared<Epoller>(this);
		if (!m_epoller->isEpollFdUseful())
		{
			LogError("init " << m_tid << " m_epoller failed");
			return false;
		}
		m_timer = std::make_shared<Timer>(this);
		m_epoller->setTimerFd(m_timer->getFd());

		// initial loop
		m_pLoop = new std::thread(
			[this]
			{
				threadIdx = m_tid;
				m_status = PRO_RUNNING;
				while (PRO_RUNNING == m_status)
				{
					// clear actCors vec
					if (m_active_tasks.size())
					{
						m_active_tasks.clear();
					}
					// get active tasks
					m_epoller->getActiveTasks(parameter::epollTimeOutMs, m_active_tasks);

					// execute new coming coroutines
					Coroutine *pNewCo = nullptr;
					while (m_pending_tasks.pop(pNewCo))
					{
						LogDebug("in exec m_pending_tasks");
						m_cors_set.insert(pNewCo);
						resume(pNewCo);
					}

					// execute the awakened coroutines
					size_t actCoCnt = m_active_tasks.size();
					for (size_t i = 0; i < actCoCnt; ++i)
					{
						resume(m_active_tasks[i]);
					}

					// clear completed coroutines
					for (auto deadCo : m_removed_cors)
					{
						m_cors_set.erase(deadCo);
						LogDebug("delete deadCo");
						{
							SpinlockGuard lock(m_cor_pool_lock);
							m_cor_pool.delete_obj(deadCo);
						}
					}
					m_removed_cors.clear();
				}
				m_status = PRO_STOPPED;
			});
		return true;
	}

	void Processor::stop()
	{
		m_status = PRO_STOPPING;
	}

	void Processor::join()
	{
		m_pLoop->join();
	}

	void Processor::killCurCo()
	{
		m_removed_cors.push_back(m_cur_coroutine);
	}

	bool Processor::isLoopThread() const
	{
		if (m_tid == threadIdx)
		{
			LogDebug("isLoopThread() return true");
			return true;
		}
		LogDebug("m_tid = " << m_tid << ", ThreadIdx = " << threadIdx << "return false");
		return false;
	}

}