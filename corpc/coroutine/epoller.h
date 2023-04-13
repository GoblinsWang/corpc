/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_EPOLLER_H
#define CORPC_COROUTINE_EPOLLER_H
#include <vector>
#include <functional>
#include <memory>
#include <sys/eventfd.h>
#include "utils.h"
#include "coroutine.h"
#include "../log/logger.h"
struct epoll_event;

namespace corpc
{

	class Processor;
	class FdEvent;

	class Epoller
	{
	public:
		using ptr = std::shared_ptr<Epoller>;

		Epoller(corpc::Processor *processor);

		~Epoller();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Epoller);

		// 修改Epoller中的事件
		bool modEvent(FdEvent *fd_event, int fd, int op);

		// 向Epoller中添加事件
		bool addEvent(FdEvent *fd_event, int fd, int op);

		// 从Epoller中移除事件
		bool delEvent(int fd);

		void addWakeupFd();

		void wakeup();

		// Obtain the activated event coroutines
		void getActiveTasks(int timeOutMs, std::vector<Coroutine *> &active_tasks);

	public:
		inline void setTimerFd(int timer_fd)
		{
			m_timer_fd = timer_fd;
		}

		inline bool isEpollFdUseful()
		{
			return m_epoll_fd < 0 ? false : true;
		}

	public:
		// already care events
		std::vector<int> m_fds;

	private:
		int m_timer_fd;

		int m_wake_fd{-1};

		int m_epoll_fd;

		// std::mutex m_mutex;

		Processor *m_processor = nullptr;

		// Store active events returned by epoll_wait
		std::vector<struct epoll_event> m_active_epoll_events;
	};

}
#endif