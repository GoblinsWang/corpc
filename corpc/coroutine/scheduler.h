/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_SCHEDULER_H
#define CORPC_COROUTINE_SCHEDULER_H
#include <vector>
#include <functional>
#include "coroutine.h"
#include "spinlock.h"
#include "processor.h"
#include "processor_selector.h"
#include "parameter.h"

namespace corpc
{

	class Scheduler
	{
	protected:
		Scheduler();

		~Scheduler();

	public:
		DISALLOW_COPY_MOVE_AND_ASSIGN(Scheduler);

		static Scheduler *getScheduler();

		// Select the corresponding processing instance based on the ID
		Processor *getProcessor(int id);

		// Without an ID, select the processing instance based on a strategy.
		Processor *getProcessor();

		void join();

	private:
		// Initialize the Scheduler with threadCnt indicating how many threads to open.
		bool startScheduler(int threadCnt);

		inline int getProCnt() const
		{
			return m_pro_cnt;
		}

	private:
		// Instance of a global scheduler.
		static Scheduler *m_scheduler;

		// The lock used for protection should not be held for a long time in principle for the sake of server performance efficiency.
		static std::mutex m_sche_mutex;

		int m_pro_cnt;

		std::vector<Processor *> m_processors;

		ProcessorSelector m_selector;
	};

}
#endif