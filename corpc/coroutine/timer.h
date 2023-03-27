/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_TIMER_H
#define CORPC_COROUTINE_TIMER_H

#include "mstime.h"
#include "utils.h"

#include <map>
#include <queue>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>

#define TIMER_DUMMYBUF_SIZE 1024

namespace corpc
{
	class Coroutine;

	class Epoller;

	// 定时器
	class Timer
	{
	public:
		using ptr = std::shared_ptr<Timer>;

		using TimerHeap = std::priority_queue<std::pair<Time, Coroutine *>, std::vector<std::pair<Time, Coroutine *>>, std::greater<std::pair<Time, Coroutine *>>>;

		Timer(Epoller *);

		~Timer();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Timer);

		int getTimeFd();

		// 获取所有已经超时的需要执行的函数
		void getExpiredCoroutines(std::vector<Coroutine *> &expiredCoroutines);

		// 在time时刻需要恢复协程co
		void runAt(Time time, Coroutine *pCo);

		// 经过time毫秒恢复协程co
		void runAfter(Time time, Coroutine *pCo);

		void wakeUp();

		// 给timefd重新设置时间，time是绝对时间
		bool resetTimeOfTimefd(Time time);

		inline bool isTimeFdUseful() { return m_timeFd < 0 ? false : true; };

	private:
		int m_timeFd;

		// 用于read timefd上数据的
		char dummyBuf_[TIMER_DUMMYBUF_SIZE];

		// 定时器协程集合
		// std::multimap<Time, Coroutine*> timerCoMap_;
		TimerHeap m_timerCoHeap;
	};

}
#endif
