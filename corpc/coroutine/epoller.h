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
#include "../log/logger.h"
struct epoll_event;

namespace corpc
{
	class Coroutine;

	class Processor;

	class Epoller
	{
	public:
		using ptr = std::shared_ptr<Epoller>;

		Epoller(corpc::Processor *processor);

		~Epoller();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Epoller);

		void setTimerfd(int timer_fd);

		// 修改Epoller中的事件
		bool modEvent(Coroutine *pCo, int fd, int interesEv);

		// 向Epoller中添加事件
		bool addEvent(Coroutine *pCo, int fd, int interesEv);

		// 从Epoller中移除事件
		bool delEvent(Coroutine *pCo, int fd, int interesEv);

		void addWakeupFd();

		void wakeup();

		// 获取被激活的事件服务
		void getActiveTasks(int timeOutMs, std::vector<Coroutine *> &activeCors);

		inline bool isEpollFdUseful() { return m_epollFd < 0 ? false : true; };

	private:
		int m_timer_fd;

		int m_wake_fd{-1}; // wakeup fd

		int m_epollFd;

		std::vector<int> m_fds; // alrady care events

		std::mutex m_mutex;

		corpc::Processor *m_processor = nullptr;

		std::vector<struct epoll_event> m_activeEpollEvents;
	};

}
#endif