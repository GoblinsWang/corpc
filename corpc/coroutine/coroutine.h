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

		Coroutine(size_t stackSize, std::function<void()> &&);

		Coroutine(size_t stackSize, std::function<void()> &);

		~Coroutine();

		DISALLOW_COPY_MOVE_AND_ASSIGN(Coroutine);

		// 恢复运行当前协程
		void resume();

		// 暂停运行当前协程
		void yield();

		Processor *getMyProcessor() { return pMyProcessor_; }

		// 运行该协程的函数
		inline void startFunc() { coFunc_(); };

		// 获取该协程的上下文
		inline Context *getCtx() { return &ctx_; }

	private:
		std::function<void()> coFunc_;

		int status_;

		Context ctx_;

		Processor *pMyProcessor_;
	};

}

#endif