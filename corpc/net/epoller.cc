/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "epoller.h"
#include "fd_event.h"
#include "../coroutine/coroutine.h"
#include "../coroutine/parameter.h"

#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace corpc
{

	Epoller::Epoller()
		: epollFd_(-1), activeEpollEvents_(parameter::epollerListFirstSize)
	{
	}

	Epoller::~Epoller()
	{
		if (isEpollFdUseful())
		{
			::close(epollFd_);
		}
	};

	bool Epoller::init(corpc::Processor *processor)
	{
		epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);
		m_processor = processor;
		return isEpollFdUseful();
	}

	void Epoller::setTimerfd(int timer_fd)
	{
		m_timer_fd = timer_fd;
	}

	// 向Epoller中添加或修改事件
	bool Epoller::addEv(int op, int fd, epoll_event event)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}

		if (::epoll_ctl(epollFd_, op, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	// 从Epoller中移除事件
	bool Epoller::removeEv(int op, int fd)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}

		if (::epoll_ctl(epollFd_, op, fd, nullptr) < 0)
		{
			return false;
		}
		return true;
	}

	void Epoller::getPendingTasks(int timeOutMs, std::vector<std::function<void()>> &pending_tasks)
	{

		int actEvNum = ::epoll_wait(epollFd_, &*activeEpollEvents_.begin(), static_cast<int>(activeEpollEvents_.size()), timeOutMs);
		LogDebug("epoll_wait back");

		if (actEvNum < 0)
		{
			LogError("epoll_wait error, skip");
		}
		else
		{
			// DebugLog << "epoll_wait back, rt = " << rt;
			for (int i = 0; i < actEvNum; ++i)
			{
				epoll_event one_event = activeEpollEvents_[i];

				corpc::FdEvent *ptr = (corpc::FdEvent *)one_event.data.ptr;
				if (ptr != nullptr)
				{
					int fd = ptr->getFd();

					if ((!(one_event.events & EPOLLIN)) && (!(one_event.events & EPOLLOUT)))
					{
						LogError("socket [" << fd << "] occur other unknow event:[" << one_event.events << "], need unregister this socket");
						m_processor->delEventInLoopThread(fd);
					}
					else
					{

						std::function<void()> read_cb;
						std::function<void()> write_cb;
						read_cb = ptr->getCallBack(READ);
						write_cb = ptr->getCallBack(WRITE);
						// if timer event, direct excute
						if (fd == m_timer_fd)
						{
							read_cb();
							continue;
						}
						if (one_event.events & EPOLLIN)
						{
							LogDebug("socket [" << fd << "] occur read event");
							std::lock_guard<std::mutex> lock(m_mutex);
							pending_tasks.push_back(read_cb);
						}
						if (one_event.events & EPOLLOUT)
						{
							LogDebug("socket [" << fd << "] occur write event");
							std::lock_guard<std::mutex> lock(m_mutex);
							pending_tasks.push_back(write_cb);
						}
					}
				}
			}
		}
	}
}