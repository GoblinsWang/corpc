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
	__thread Processor *t_processor_ptr = nullptr;
	__thread int threadIdx = -1;

	Processor::Processor(int tid)
		: m_tid(tid), m_status(PRO_STOPPED), m_pLoop(nullptr), m_pCurCoroutine(nullptr), mainCtx_(0)
	{
		mainCtx_.makeCurContext();
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
		for (auto co : m_coSet)
		{
			delete co;
		}
	}
	// wait event on fd
	void Processor::waitEvent(FdEvent::ptr fd_event, int fd, int event)
	{
		// LogDebug("in waitEvent, fd = " << fd);

		fd_event->setCoroutine(m_pCurCoroutine);
		m_epoller->addEvent(fd_event.get(), fd, event);

		yield();

		m_epoller->delEvent(fd);
		fd_event->clearCoroutine();
	}

	void Processor::addTask(std::function<void()> task, bool is_wakeup /*=true*/)
	{
		LogDebug("in addTask()");
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_pending_tasks.push_back(task);
		}
		if (is_wakeup)
		{
			m_epoller->wakeup();
		}
	}

	void Processor::addTask(std::vector<std::function<void()>> task, bool is_wakeup /* =true*/)
	{
		LogDebug("in addTask()");
		if (task.size() == 0)
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_pending_tasks.insert(m_pending_tasks.end(), task.begin(), task.end());
		}
		if (is_wakeup)
		{
			m_epoller->wakeup();
		}
	}

	void Processor::addCoroutine(corpc::Coroutine *pCo, bool is_wakeup /*=true*/)
	{
		pCo->setProcessor(this);
		m_newCoroutines.push(pCo);

		if (is_wakeup)
		{
			LogDebug("in processor " << m_tid << ", to wakeup epoller");
			m_epoller->wakeup();
		}
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

	void Processor::resume(Coroutine *pCo)
	{
		if (nullptr == pCo)
		{
			return;
		}

		if (m_coSet.find(pCo) == m_coSet.end())
		{
			return;
		}

		m_pCurCoroutine = pCo;
		pCo->resume();
	}

	void Processor::yield()
	{
		m_pCurCoroutine->yield();
		/*
			切换到主线程，然后将当前协程的上下文保存到当前协程的对象里面
		*/
		mainCtx_.swapToMe(m_pCurCoroutine->getCtx());
	}

	void Processor::wait(Time time)
	{
		m_pCurCoroutine->yield();
		m_timer->runAfter(time, m_pCurCoroutine);
		/*
			切换到主线程，然后将当前协程的上下文保存到当前协程的对象里面
		*/
		mainCtx_.swapToMe(m_pCurCoroutine->getCtx());
	}

	void Processor::goCo(Coroutine *pCo)
	{
		m_newCoroutines.push(pCo);
		m_epoller->wakeup();
	}

	void Processor::goCoBatch(std::vector<Coroutine *> &cos)
	{
		{
			SpinlockGuard lock(newQueLock_);
			for (auto pCo : cos)
			{
				m_newCoroutines.push(pCo);
			}
		}
		m_epoller->wakeup();
	}

	Processor *Processor::GetProcessor()
	{
		return t_processor_ptr;
	}

	bool Processor::loop()
	{
		m_epoller = std::make_shared<Epoller>(this);
		if (!m_epoller->isEpollFdUseful())
		{
			LogError("init " << m_tid << " m_epoller failed");
			return false;
		}
		m_timer = std::make_shared<Timer>(m_epoller.get());
		if (!m_timer->isTimeFdUseful())
		{
			LogError("init " << m_tid << " m_timer failed");
			return false;
		}
		m_epoller->setTimerfd(m_timer->getTimeFd());

		// 初始化loop
		m_pLoop = new std::thread(
			[this]
			{
				threadIdx = m_tid;
				m_status = PRO_RUNNING;
				while (PRO_RUNNING == m_status)
				{
					// 清空所有列表
					if (m_actCoroutines.size())
					{
						m_actCoroutines.clear();
					}
					if (m_timerExpiredCo.size())
					{
						m_timerExpiredCo.clear();
					}
					// get active tasks
					m_epoller->getActiveTasks(parameter::epollTimeOutMs, m_actCoroutines);

					// get timeout coroutine
					m_timer->getExpiredCoroutines(m_timerExpiredCo);
					size_t timerCoCnt = m_timerExpiredCo.size();
					// if (timerCoCnt)
					// 	LogTrace("the num of timeout coroutine : " << timerCoCnt);
					for (size_t i = 0; i < timerCoCnt; ++i)
					{
						resume(m_timerExpiredCo[i]);
					}

					// 执行新来的协程
					Coroutine *pNewCo = nullptr;
					while (m_newCoroutines.pop(pNewCo))
					{

						m_coSet.insert(pNewCo);
						resume(pNewCo);
					}
					// LogTrace("m_coSet : " << KV(m_coSet.size()));
					// 执行被唤醒的协程
					size_t actCoCnt = m_actCoroutines.size();
					for (size_t i = 0; i < actCoCnt; ++i)
					{
						resume(m_actCoroutines[i]);
					}

					// 清除已经执行完毕的协程
					for (auto deadCo : m_removedCo)
					{
						m_coSet.erase(deadCo);
						// delete deadCo;
						{
							SpinlockGuard lock(m_coPoolLock);
							m_copool.delete_obj(deadCo);
						}
					}
					m_removedCo.clear();
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
		m_removedCo.push_back(m_pCurCoroutine);
	}

}