/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "timer.h"
#include "coroutine.h"
#include "epoller.h"
#include "../net/fd_event.h"
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>

namespace corpc
{

	Timer::Timer(Epoller *pEpoller)
	{
		m_timeFd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
		auto fd_event = corpc::FdEventContainer::GetFdContainer()->getFdEvent(m_timeFd);
		pEpoller->addEvent(fd_event.get(), m_timeFd, EPOLLIN | EPOLLPRI | EPOLLRDHUP);
	}

	Timer::~Timer()
	{
		if (isTimeFdUseful())
		{
			::close(m_timeFd);
		}
	}

	int Timer::getTimeFd()
	{
		return m_timeFd;
	}

	void Timer::getExpiredCoroutines(std::vector<Coroutine *> &expiredCoroutines)
	{
		Time nowTime = Time::now();
		while (!m_timerCoHeap.empty() && m_timerCoHeap.top().first <= nowTime)
		{
			expiredCoroutines.push_back(m_timerCoHeap.top().second);
			m_timerCoHeap.pop();
		}

		if (!expiredCoroutines.empty())
		{
			ssize_t cnt = TIMER_DUMMYBUF_SIZE;
			while (cnt >= TIMER_DUMMYBUF_SIZE)
			{
				cnt = ::read(m_timeFd, dummyBuf_, TIMER_DUMMYBUF_SIZE);
			}
		}

		if (!m_timerCoHeap.empty())
		{
			Time time = m_timerCoHeap.top().first;
			resetTimeOfTimefd(time);
		}
	}

	void Timer::runAt(Time time, Coroutine *pCo)
	{
		m_timerCoHeap.push(std::move(std::pair<Time, Coroutine *>(time, pCo)));
		if (m_timerCoHeap.top().first == time)
		{
			// 新加入的任务是最紧急的任务则需要更改timefd所设置的时间
			resetTimeOfTimefd(time);
		}
	}

	// 给timefd重新设置时间，time是绝对时间
	bool Timer::resetTimeOfTimefd(Time time)
	{
		struct itimerspec newValue;
		struct itimerspec oldValue;
		memset(&newValue, 0, sizeof newValue);
		memset(&oldValue, 0, sizeof oldValue);
		newValue.it_value = time.timeIntervalFromNow();
		int ret = ::timerfd_settime(m_timeFd, 0, &newValue, &oldValue); // 第二个参数为0表示相对时间，为1表示绝对时间
		return ret < 0 ? false : true;
	}

	void Timer::runAfter(Time time, Coroutine *pCo)
	{
		Time runTime(Time::now().getTimeVal() + time.getTimeVal());
		runAt(runTime, pCo);
	}

	void Timer::wakeUp()
	{
		resetTimeOfTimefd(Time::now());
	}
}