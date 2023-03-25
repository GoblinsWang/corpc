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

	Epoller::Epoller(corpc::Processor *processor)
		: m_epollFd(-1), m_activeEpollEvents(parameter::epollerListFirstSize)
	{
		m_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
		m_processor = processor;
		if ((m_wake_fd = eventfd(0, EFD_NONBLOCK)) <= 0)
		{
			LogError("start server error. event_fd error");
			_exit(0);
		}
		LogDebug("wakefd = " << m_wake_fd);
		addWakeupFd();
	}

	Epoller::~Epoller()
	{
		if (isEpollFdUseful())
		{
			::close(m_epollFd);
		}
	};

	void Epoller::setTimerfd(int timer_fd)
	{
		m_timer_fd = timer_fd;
	}

	// 修改Epoller中的事件
	bool Epoller::modEvent(Coroutine *pCo, int fd, int op)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}
		LogDebug("modEvent, fd = " << fd);
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = fd;
		event.events = op;
		event.data.ptr = pCo;
		if (::epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	// 向Epoller中添加事件
	bool Epoller::addEvent(Coroutine *pCo, int fd, int op)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}
		LogDebug("addEvent, fd = " << fd);
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = fd;
		event.events = op;
		// event.data.ptr = pCo;
		if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	// 从Epoller中移除事件
	bool Epoller::delEvent(Coroutine *pCo, int fd, int op)
	{
		if (!isEpollFdUseful())
		{
			return false;
		}

		if (::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) < 0)
		{
			return false;
		}
		return true;
	}

	void Epoller::addWakeupFd()
	{
		if (!addEvent(nullptr, m_wake_fd, EPOLLIN))
		{
			LogError("epoo_ctl error, fd[" << m_wake_fd << "], errno=" << errno);
		}
		m_fds.push_back(m_wake_fd);
	}

	void Epoller::wakeup()
	{
		LogDebug("wakeup fd = " << m_wake_fd);
		uint64_t tmp = 1;
		uint64_t *p = &tmp;
		if (::write(m_wake_fd, p, 8) != 8)
		{
			LogError("write wakeupfd[" << m_wake_fd << "] error");
		}
	}

	void Epoller::getActiveTasks(int timeOutMs, std::vector<Coroutine *> &activeCors)
	{

		int actEvNum = ::epoll_wait(m_epollFd, &*m_activeEpollEvents.begin(), static_cast<int>(m_activeEpollEvents.size()), timeOutMs);
		// LogDebug("epoll_wait back, actEvNum = " << actEvNum);

		if (actEvNum < 0)
		{
			LogError("epoll_wait error, skip");
		}
		else
		{
			for (int i = 0; i < actEvNum; ++i)
			{
				epoll_event one_event = m_activeEpollEvents[i];
				// LogDebug("fd = " << one_event.data.fd);
				if (one_event.data.fd == m_wake_fd && (one_event.events & READ))
				{
					// wakeup
					LogDebug("epoll wakeup, fd=[" << m_wake_fd << "]");
					char buf[8];
					while (1)
					{
						if ((::read(m_wake_fd, buf, 8) == -1) && errno == EAGAIN)
						{
							break;
						}
					}
				}
				else
				{
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
							if (one_event.events & EPOLLIN)
							{
								// LogDebug("socket [" << fd << "] occur read event");
								activeCors.push_back(ptr->getCoroutine());
							}
							if (one_event.events & EPOLLOUT)
							{
								// LogDebug("socket [" << fd << "] occur write event");
								activeCors.push_back(ptr->getCoroutine());
							}
						}
					}
				}
			}
			if (actEvNum == static_cast<int>(m_activeEpollEvents.size()))
			{
				// 若从epoll中获取事件的数组满了，说明这个数组的大小可能不够，扩展一倍
				m_activeEpollEvents.resize(m_activeEpollEvents.size() * 2);
			}
		}
	}
}