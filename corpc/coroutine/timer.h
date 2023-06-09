/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_TIMER_H
#define CORPC_COROUTINE_TIMER_H

#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <functional>
#include "utils.h"
#include "fd_event.h"
#include "coroutine"
#include "rw_mutex.h"
#include "../log/logger.h"

namespace corpc
{
	int64_t getNowMs();

	class TimerEvent
	{

	public:
		using ptr = std::shared_ptr<TimerEvent>;
		TimerEvent(int64_t interval, bool is_repeated, std::function<void()> task)
			: m_interval(interval), m_is_repeated(is_repeated), m_task(task)
		{
			m_arrive_time = getNowMs() + m_interval;
			// LogDebug("timeevent will occur at " << m_arrive_time);
		}

		void resetTime()
		{

			m_arrive_time = getNowMs() + m_interval;
			m_is_cancled = false;
		}

		void wake()
		{
			m_is_cancled = false;
		}

		void cancle()
		{
			m_is_cancled = true;
		}

		void cancleRepeated()
		{
			m_is_repeated = false;
		}

	public:
		int64_t m_arrive_time; // when to excute task, ms
		int64_t m_interval;	   // interval between two tasks, ms
		bool m_is_repeated{false};
		bool m_is_cancled{false};
		// Coroutine *m_coroutine{nullptr};
		std::function<void()> m_task;
	};

	class Processor;

	class Timer : public corpc::FdEvent
	{
	public:
		using ptr = std::shared_ptr<Timer>;

		Timer(Processor *processor);

		~Timer();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Timer);

		// Add a new scheduled task
		void addTimerEvent(TimerEvent::ptr event, bool need_reset = true);

		// Delete an old scheduled task
		void delTimerEvent(TimerEvent::ptr event);

		// To execute tasks that have reached their expiration date
		void onTimer();

		void resetArriveTime();

	private:
		Processor *m_processor = nullptr;

		RWMutex m_event_mutex;

		// Collection of timer events
		std::multimap<int64_t, TimerEvent::ptr> m_pending_events;
	};

}
#endif
