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

		// 恢复运行当前协程
		void resume();

		// 暂停运行当前协程
		void yield();

		void setProcessor(Processor *);

		Processor *getMyProcessor() { return m_processor; }

		// 运行该协程的函数
		inline void startFunc() { m_coFunc(); };

		// 获取该协程的上下文
		inline Context *getCtx() { return &m_ctx; }

	private:
		std::function<void()> m_coFunc;

		int m_status;

		Context m_ctx;

		Processor *m_processor;
	};

}

#endif