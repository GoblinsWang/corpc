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
		: m_tid(tid), status_(PRO_STOPPED), pLoop_(nullptr), runningNewQue_(0), pCurCoroutine_(nullptr), mainCtx_(0)
	{
		mainCtx_.makeCurContext();
	}

	Processor::~Processor()
	{
		if (PRO_RUNNING == status_)
		{
			stop();
		}
		if (PRO_STOPPING == status_)
		{
			join();
		}
		if (nullptr != pLoop_)
		{
			delete pLoop_;
		}
		for (auto co : coSet_)
		{
			delete co;
		}
	}

	// call by other threads, need lock
	void Processor::addEvent(int fd, epoll_event event, bool is_wakeup /*=true*/)
	{
		if (fd == -1)
		{
			LogError("add error. fd invalid, fd = -1");
			return;
		}
		if (isLoopThread())
		{
			addEventInLoopThread(fd, event);
			return;
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_pending_add_fds.insert(std::pair<int, epoll_event>(fd, event));
		}
		if (is_wakeup)
		{
			wakeUpEpoller();
		}
	}

	// call by other threads, need lock
	void Processor::delEvent(int fd, bool is_wakeup /*=true*/)
	{

		if (fd == -1)
		{
			LogError("add error. fd invalid, fd = -1");
			return;
		}

		if (isLoopThread())
		{
			delEventInLoopThread(fd);
			return;
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_pending_del_fds.push_back(fd);
		}
		if (is_wakeup)
		{
			wakeUpEpoller();
		}
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
			LogDebug("to wakeUpEpoller()");
			wakeUpEpoller();
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
			wakeUpEpoller();
		}
	}

	void Processor::addCoroutine(corpc::Coroutine *cor, bool is_wakeup /*=true*/)
	{
		LogDebug("in addCoroutine()");
		auto func = [this, cor]()
		{
			resume(cor);
		};
		addTask(func, is_wakeup);
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

	// need't mutex, only this thread call
	void Processor::addEventInLoopThread(int fd, epoll_event event)
	{

		assert(isLoopThread());

		int op = EPOLL_CTL_ADD;
		bool is_add = true;
		// int tmp_fd = event;
		auto it = find(m_fds.begin(), m_fds.end(), fd);
		if (it != m_fds.end())
		{
			is_add = false;
			op = EPOLL_CTL_MOD;
		}

		if (!m_epoller.addEv(op, fd, event))
		{
			LogDebug("epoo_ctl error, fd[" << fd << "]");
			return;
		}
		if (is_add)
		{
			m_fds.push_back(fd);
		}
		LogDebug("epoll_ctl add succ, fd[" << fd << "]");
	}

	// need't mutex, only this thread call
	void Processor::delEventInLoopThread(int fd)
	{

		assert(isLoopThread());

		auto it = find(m_fds.begin(), m_fds.end(), fd);
		if (it == m_fds.end())
		{
			LogDebug("fd[" << fd << "] not in this loop");
			return;
		}
		int op = EPOLL_CTL_DEL;

		if (!m_epoller.removeEv(op, fd))
		{
			LogDebug("epoo_ctl error, fd[" << fd << "]");
		}

		m_fds.erase(it);
		LogDebug("del succ, fd[" << fd << "]");
	}

	void Processor::resume(Coroutine *pCo)
	{
		if (nullptr == pCo)
		{
			return;
		}

		if (coSet_.find(pCo) == coSet_.end())
		{
			return;
		}

		pCurCoroutine_ = pCo;
		pCo->resume();
	}

	void Processor::yield()
	{
		pCurCoroutine_->yield();
		/*
			切换到主线程，然后将当前协程的上下文保存到当前协程的对象里面
		*/
		mainCtx_.swapToMe(pCurCoroutine_->getCtx());
	}

	void Processor::wait(Time time)
	{
		pCurCoroutine_->yield();
		m_timer.runAfter(time, pCurCoroutine_);
		/*
			切换到主线程，然后将当前协程的上下文保存到当前协程的对象里面
		*/
		mainCtx_.swapToMe(pCurCoroutine_->getCtx());
	}

	void Processor::goCo(Coroutine *pCo)
	{
		{
			SpinlockGuard lock(newQueLock_);
			newCoroutines_[!runningNewQue_].push(pCo);
		}
		wakeUpEpoller();
	}

	void Processor::goCoBatch(std::vector<Coroutine *> &cos)
	{
		{
			SpinlockGuard lock(newQueLock_);
			for (auto pCo : cos)
			{
				newCoroutines_[!runningNewQue_].push(pCo);
			}
		}
		wakeUpEpoller();
	}

	Processor *Processor::GetProcessor()
	{
		return t_processor_ptr;
	}

	bool Processor::loop()
	{
		// 初始化Epoller
		if (!m_epoller.init(this))
		{
			return false;
		}

		// 初始化Timer
		if (!m_timer.init(&m_epoller))
		{
			return false;
		}

		// 初始化loop
		pLoop_ = new std::thread(
			[this]
			{
				threadIdx = m_tid;
				status_ = PRO_RUNNING;
				while (PRO_RUNNING == status_)
				{
					// 清空所有列表
					if (m_pending_tasks.size())
					{
						m_pending_tasks.clear();
					}
					if (timerExpiredCo_.size())
					{
						timerExpiredCo_.clear();
					}
					// 获取活跃事件
					m_epoller.getPendingTasks(parameter::epollTimeOutMs, m_pending_tasks);

					// 处理超时协程
					m_timer.getExpiredCoroutines(timerExpiredCo_);
					size_t timerCoCnt = timerExpiredCo_.size();
					for (size_t i = 0; i < timerCoCnt; ++i)
					{
						resume(timerExpiredCo_[i]);
					}

					// 执行新来的协程
					Coroutine *pNewCo = nullptr;
					int runningQue = runningNewQue_;

					while (!newCoroutines_[runningQue].empty())
					{
						{
							pNewCo = newCoroutines_[runningQue].front();
							newCoroutines_[runningQue].pop();
							coSet_.insert(pNewCo);
						}
						resume(pNewCo);
					}

					{
						SpinlockGuard lock(newQueLock_);
						runningNewQue_ = !runningQue;
					}

					// 执行被唤醒的协程
					size_t actCoCnt = actCoroutines_.size();
					for (size_t i = 0; i < actCoCnt; ++i)
					{
						resume(actCoroutines_[i]);
					}

					// 清除已经执行完毕的协程
					for (auto deadCo : removedCo_)
					{
						coSet_.erase(deadCo);
						// delete deadCo;
						{
							SpinlockGuard lock(coPoolLock_);
							m_copool.delete_obj(deadCo);
						}
					}
					removedCo_.clear();
				}
				status_ = PRO_STOPPED;
			});
		return true;
	}

	void Processor::stop()
	{
		status_ = PRO_STOPPING;
	}

	void Processor::join()
	{
		pLoop_->join();
	}

	void Processor::wakeUpEpoller()
	{
		m_timer.wakeUp();
	}

	void Processor::killCurCo()
	{
		removedCo_.push_back(pCurCoroutine_);
	}

}