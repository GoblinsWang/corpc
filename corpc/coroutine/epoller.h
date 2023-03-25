/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_EPOLLER_H
#define CORPC_COROUTINE_EPOLLER_H
#include "utils.h"
#include "../log/logger.h"
#include <vector>
#include <functional>

struct epoll_event;

namespace corpc
{
	class Coroutine;

	class Processor;

	class Epoller
	{
	public:
		Epoller();
		~Epoller();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Epoller);

		// 要使用EventEpoller必须调用该函数初始化，失败则返回false
		bool init(corpc::Processor *processor);

		void setTimerfd(int timer_fd);

		// 修改Epoller中的事件
		bool modEvent(Coroutine *pCo, int fd, int interesEv);

		// 向Epoller中添加事件
		bool addEvent(Coroutine *pCo, int fd, int interesEv);

		// 从Epoller中移除事件
		bool delEvent(Coroutine *pCo, int fd, int interesEv);

		// 获取被激活的事件服务
		void getActiveTasks(int timeOutMs, std::vector<Coroutine *> &activeCors);

	private:
		inline bool isEpollFdUseful() { return epollFd_ < 0 ? false : true; };

	private:
		int m_timer_fd;

		int epollFd_;

		std::mutex m_mutex;

		corpc::Processor *m_processor = nullptr;

		std::vector<struct epoll_event> activeEpollEvents_;
	};

}
#endif