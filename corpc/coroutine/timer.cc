/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "timer.h"
#include "coroutine.h"
#include "processor.h"

namespace corpc
{

	int64_t getNowMs()
	{
		timeval val;
		gettimeofday(&val, nullptr);
		int64_t re = val.tv_sec * 1000 + val.tv_usec / 1000;
		return re;
	}

	Timer::Timer(Processor *processor)
		: FdEvent(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)), m_processor(processor)
	{
		LogDebug("m_timer fd = " << m_fd);
		if (m_fd == -1)
		{
			LogError("timerfd_create error");
		}
		m_processor->getEpoller()->addEvent(this, m_fd, EPOLLIN | EPOLLPRI | EPOLLRDHUP);
	}

	Timer::~Timer()
	{
		if (m_fd > 0)
		{
			m_processor->getEpoller()->delEvent(m_fd);
			::close(m_fd);
		}
	}

	void Timer::addTimerEvent(TimerEvent::ptr event, bool need_reset /*=true*/)
	{
		bool is_reset = false;

		m_event_mutex.wlock(); // 写锁
		if (m_pending_events.empty())
		{
			is_reset = true;
		}
		else
		{
			auto it = m_pending_events.begin();
			int64_t now = getNowMs();
			if (event->m_arrive_time < (*it).second->m_arrive_time or (*it).second->m_arrive_time < now)
			{
				is_reset = true;
			}
		}
		// LogDebug("m_pending_events size : " << m_pending_events.size() << ", is_reset : " << is_reset);
		m_pending_events.emplace(event->m_arrive_time, event);
		m_event_mutex.wunlock(); // 释放写锁

		if (is_reset && need_reset)
		{
			// LogDebug("need reset timer");
			resetArriveTime();
		}
		// LogInfo("add timer event succ");
	}

	void Timer::delTimerEvent(TimerEvent::ptr event)
	{
		event->m_is_cancled = true;
		m_event_mutex.wlock(); // 写锁
		auto begin = m_pending_events.lower_bound(event->m_arrive_time);
		auto end = m_pending_events.upper_bound(event->m_arrive_time);
		auto it = begin;
		for (it = begin; it != end; it++)
		{
			if (it->second == event)
			{
				LogDebug("find timer event, now delete it. src arrive time=" << event->m_arrive_time);
				break;
			}
		}
		if (it != m_pending_events.end())
		{
			m_pending_events.erase(it);
		}
		m_event_mutex.wunlock(); // 释放写锁
		LogDebug("del timer event succ, origin arrvite time=" << event->m_arrive_time);
	}

	void Timer::onTimer()
	{
		// LogDebug("onTimer, first read data");
		char buf[8];
		while (1)
		{
			if ((::read(m_fd, buf, 8) == -1) && errno == EAGAIN)
			{
				break;
			}
		}
		std::vector<TimerEvent::ptr> tmps;
		std::vector<std::pair<int64_t, std::function<void()>>> tasks;
		int64_t now = getNowMs();

		m_event_mutex.wlock(); // 写锁
		auto it = m_pending_events.begin();
		for (it = m_pending_events.begin(); it != m_pending_events.end(); ++it)
		{
			if ((*it).first <= now && !((*it).second->m_is_cancled))
			{
				tmps.push_back((*it).second);
				tasks.push_back(std::make_pair((*it).second->m_arrive_time, (*it).second->m_task));
			}
			else
			{
				break;
			}
		}
		m_pending_events.erase(m_pending_events.begin(), it);
		m_event_mutex.wunlock(); // 释放写锁

		for (auto i = tmps.begin(); i != tmps.end(); ++i)
		{
			// LogDebug("excute timer event on " << (*i)->m_arrive_time);
			if ((*i)->m_is_repeated)
			{
				(*i)->resetTime();
				addTimerEvent(*i, false);
			}
		}

		resetArriveTime();

		for (auto t : tasks)
		{
			// LogInfo("excute timeevent:" << t.first);
			t.second();
		}
	}

	void Timer::resetArriveTime()
	{
		std::multimap<int64_t, TimerEvent::ptr> tmp;
		m_event_mutex.rlock(); // 读锁
		tmp = m_pending_events;
		m_event_mutex.runlock(); // 释放读锁
		if (tmp.size() == 0)
		{
			// LogInfo("no timerevent pending, size = 0");
			return;
		}

		int64_t now = getNowMs();
		auto it = tmp.begin();

		int64_t interval = 10;
		if ((*it).first > now)
		{
			// LogInfo("all timer events has already expire");
			interval = (*it).first - now;
		}

		itimerspec new_value;
		memset(&new_value, 0, sizeof(new_value));

		timespec ts;
		memset(&ts, 0, sizeof(ts));
		ts.tv_sec = interval / 1000;
		ts.tv_nsec = (interval % 1000) * 1000000;
		new_value.it_value = ts;

		int rt = timerfd_settime(m_fd, 0, &new_value, nullptr);

		if (rt != 0)
		{
			LogError("tiemr_settime error, interval=" << interval);
		}
		else
		{
			// LogInfo("reset timer succ, next occur time=" << (*it).first);
		}
	}
}