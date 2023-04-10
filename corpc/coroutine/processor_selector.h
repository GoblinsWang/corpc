/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_PROCESSOR_SELECTOR_H
#define CORPC_COROUTINE_PROCESSOR_SELECTOR_H
#include <vector>

namespace corpc
{
	class Processor;

	enum scheduleStrategy
	{
		MIN_EVENT_FIRST = 0, // 最少事件优先
		ROUND_ROBIN			 // 轮流分发
	};

	// The event manager selector decides which event should be placed in the next event manager.
	class ProcessorSelector
	{
	public:
		ProcessorSelector(std::vector<Processor *> &processors, int strategy = ROUND_ROBIN) : m_curpro_id(0), m_strategy(strategy), m_processors(processors) {}

		~ProcessorSelector() {}

		Processor *next();

	public:
		/*
			Set up strategies for distributing tasks.
				MIN_EVENT_FIRST : Select the processing instance with the least number of coroutines each time a new coroutine is created.
				ROUND_ROBIN : Select the processing instance in turn each time a new coroutine is created.
		*/
		inline void setStrategy(int strategy)
		{
			m_strategy = strategy;
		};

	private:
		int m_curpro_id;

		int m_strategy;

		std::vector<Processor *> &m_processors;
	};

}

#endif