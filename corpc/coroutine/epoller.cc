/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "epoller.h"
#include "coroutine.h"
#include "parameter.h"
#include "../net/fd_event.h"
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

	// 修改Epoller中的事件
	bool Epoller::modEvent(Coroutine *pCo, int fd, int interesEv)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.events = interesEv;
		event.data.ptr = pCo;
		if (::epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	// 向Epoller中添加事件
	bool Epoller::addEvent(Coroutine *pCo, int fd, int interesEv)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.events = interesEv;
		event.data.ptr = pCo;
		if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	// 从Epoller中移除事件
	bool Epoller::delEvent(Coroutine *pCo, int fd, int interesEv)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.events = interesEv;
		event.data.ptr = pCo;
		if (::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	void Epoller::getActiveTasks(int timeOutMs, std::vector<Coroutine *> &activeCors)
	{

		int actEvNum = ::epoll_wait(epollFd_, &*activeEpollEvents_.begin(), static_cast<int>(activeEpollEvents_.size()), -1);
		// LogDebug("epoll_wait back");

		if (actEvNum < 0)
		{
			LogError("epoll_wait error, skip");
		}
		else
		{
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
						// std::function<void()> read_cb;
						// std::function<void()> write_cb;
						// read_cb = ptr->getCallBack(READ);
						// write_cb = ptr->getCallBack(WRITE);
						// if timer event, direct excute
						if (fd == m_timer_fd)
						{
							uint64_t howmany;
							ssize_t n = ::read(fd, &howmany, sizeof howmany);
							continue;
						}
						if (one_event.events & EPOLLIN)
						{
							LogDebug("socket [" << fd << "] occur read event");
							activeCors.push_back(ptr->getCoroutine());
						}
						if (one_event.events & EPOLLOUT)
						{
							LogDebug("socket [" << fd << "] occur write event");
							activeCors.push_back(ptr->getCoroutine());
						}
					}
				}
			}
			if (actEvNum == static_cast<int>(activeEpollEvents_.size()))
			{
				// 若从epoll中获取事件的数组满了，说明这个数组的大小可能不够，扩展一倍
				activeEpollEvents_.resize(activeEpollEvents_.size() * 2);
			}
		}
	}
}