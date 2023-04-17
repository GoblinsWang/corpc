/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_COROUTINE_H
#define CORPC_COROUTINE_COROUTINE_H
#include <functional>
#include <memory>
#include "context.h"
#include "utils.h"
#include "../log/logger.h"

namespace corpc
{

	enum coStatus
	{
		CO_READY = 0,
		CO_RUNNING,
		CO_WAITING,
		CO_DEAD
	};

	class Processor;

	class Coroutine
	{
	public:
		using ptr = std::shared_ptr<Coroutine>;

		Coroutine(std::function<void()> &&, size_t stackSize);

		Coroutine(std::function<void()> &, size_t stackSize);

		~Coroutine();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Coroutine);

		// resume running the current coroutine
		void resume();

		// pause the current coroutine
		void yield();

		// set the processor to facilitate coroutine switching
		void setProcessor(Processor *);

	public:
		inline Processor *getMyProcessor() { return m_processor; }

		// run the function of this coroutine
		inline void startFunc() { m_coFunc(); };

		// obtain the context of the coroutine
		inline Context *getCtx() { return &m_ctx; }

	private:
		std::function<void()> m_coFunc;

		int m_status;

		Context m_ctx;

		Processor *m_processor;
	};

}

#endif