/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "coroutine.h"
#include "processor.h"
#include "parameter.h"

using namespace corpc;

static void coWrapFunc(Processor *pP)
{
	pP->getCurRunningCo()->startFunc();
	pP->killCurCo();
}

Coroutine::Coroutine(size_t stackSize, std::function<void()> &&func)
	: coFunc_(std::move(func)), status_(CO_DEAD), ctx_(stackSize)
{
	status_ = CO_READY;
}

Coroutine::Coroutine(size_t stackSize, std::function<void()> &func)
	: coFunc_(func), status_(CO_DEAD), ctx_(stackSize)
{
	status_ = CO_READY;
}

Coroutine::~Coroutine()
{
}

void Coroutine::resume()
{
	Context *pMainCtx = pMyProcessor_->getMainCtx();
	switch (status_)
	{
	/*
		CO_READY 状态一般是针对新来的协程任务，所以还没有创建过上下文，需要先创建，再进行swap切换。
	*/
	case CO_READY:
		status_ = CO_RUNNING;
		ctx_.makeContext((void (*)(void))coWrapFunc, pMyProcessor_, pMainCtx);
		ctx_.swapToMe(pMainCtx);
		break;
	/*
		CO_WAITING 状态是之前被挂起的，挂起的时候协程内是保存了当时的上下文信息的，不需要重新makeContext，所以可以直接swap
	*/
	case CO_WAITING:
		status_ = CO_RUNNING;
		ctx_.swapToMe(pMainCtx);
		break;

	default:
		break;
	}
}

void Coroutine::yield()
{
	status_ = CO_WAITING;
};