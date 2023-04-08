/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "epoller.h"
#include "coroutine.h"
#include "parameter.h"
#include "fd_event.h"
#include "processor.h"
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <functional>
#include <algorithm>
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
		// LogDebug("wakefd = " << m_wake_fd);
		addWakeupFd();
	}

	Epoller::~Epoller()
	{
		if (isEpollFdUseful())
		{
			::close(m_epollFd);
		}
	};

	// 修改Epoller中的事件
	bool Epoller::modEvent(FdEvent *fd_event, int fd, int op)
	{
		auto it = find(m_fds.begin(), m_fds.end(), fd);
		assert(it != m_fds.end());

		LogDebug("modEvent, fd = " << fd);
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.events = op;
		event.data.ptr = fd_event;
		if (::epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event) < 0)
		{
			return false;
		}
		return true;
	}

	// 向Epoller中添加事件
	bool Epoller::addEvent(FdEvent *fd_event, int fd, int op)
	{
		auto it = find(m_fds.begin(), m_fds.end(), fd);
		assert(it == m_fds.end());

		LogDebug("addEvent, fd = " << fd);
		struct epoll_event event;
		memset(&event, 0, sizeof(event));
		event.events = op;
		event.data.ptr = fd_event;
		if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) < 0)
		{
			return false;
		}
		m_fds.push_back(fd);

		return true;
	}

	// 从Epoller中移除事件
	bool Epoller::delEvent(int fd)
	{
		auto it = find(m_fds.begin(), m_fds.end(), fd);
		assert(it != m_fds.end());
		LogTrace("delEvent, fd = " << fd);

		if (::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) < 0)
		{
			LogError("epoo_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno));
			return false;
		}
		m_fds.erase(it);
		return true;
	}

	void Epoller::addWakeupFd()
	{
		corpc::FdEvent::ptr fd_event = corpc::FdEventContainer::GetFdContainer()->getFdEvent(m_wake_fd);
		if (!addEvent(fd_event.get(), m_wake_fd, EPOLLIN))
		{
			LogError("epoo_ctl error, fd[" << m_wake_fd << "], errno=" << errno);
		}
		m_fds.push_back(m_wake_fd);
	}

	void Epoller::wakeup()
	{
		// LogDebug("wakeup fd = " << m_wake_fd);
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
		// LogInfo("epoll_wait back, actEvNum = " << actEvNum);

		if (actEvNum < 0)
		{
			LogError("epoll_wait error, skip");
		}
		else
		{
			for (int i = 0; i < actEvNum; ++i)
			{
				epoll_event one_event = m_activeEpollEvents[i];
				corpc::FdEvent *ptr = (corpc::FdEvent *)one_event.data.ptr;
				int fd = ptr->getFd();
				// LogDebug("fd = " << one_event.data.fd);
				if (fd == m_wake_fd && (one_event.events & READ))
				{
					// wakeup
					LogDebug(" is m_wake_fd, fd = [" << m_wake_fd << "]");
					char buf[8];
					while (1)
					{
						// LogDebug(" is m_wake_fd, fd = [" << m_wake_fd << "]");
						if ((::read(m_wake_fd, buf, 8) == -1) && errno == EAGAIN)
						{
							break;
						}
					}
				}
				else if (fd == m_timer_fd && (one_event.events & READ))
				{
					// LogDebug(" is m_timer_fd, fd=[" << m_wake_fd << "]");
					// TODO：
					m_processor->getTimer()->onTimer();
				}
				else
				{
					// LogDebug("not m_wake_fd, fd=[" << fd << "]");

					if ((!(one_event.events & READ)) && (!(one_event.events & WRITE)))
					{
						LogError("socket [" << fd << "] occur other unknow event:[" << one_event.events << "], need unregister this socket");
						delEvent(fd);
					}
					else
					{
						if (one_event.events & READ)
						{
							// LogDebug("socket [" << fd << "] occur read event");
							activeCors.push_back(ptr->getCoroutine());
						}
						if (one_event.events & WRITE)
						{
							// LogDebug("socket [" << fd << "] occur write event");
							activeCors.push_back(ptr->getCoroutine());
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