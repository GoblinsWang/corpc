/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "coroutine.h"
#include "processor.h"
#include "parameter.h"

namespace corpc
{

	static void coWrapFunc(Processor *pP)
	{
		pP->getCurRunningCo()->startFunc();
		pP->killCurCo();
		LogDebug("after kill curco");
		pP->getMainCtx()->swapToMe(pP->getCurRunningCo()->getCtx());
	}

	Coroutine::Coroutine(std::function<void()> &&func, size_t stackSize)
		: m_coFunc(std::move(func)), m_status(CO_DEAD), m_ctx(stackSize)
	{
		m_status = CO_READY;
	}

	Coroutine::Coroutine(std::function<void()> &func, size_t stackSize)
		: m_coFunc(func), m_status(CO_DEAD), m_ctx(stackSize)
	{
		m_status = CO_READY;
	}

	Coroutine::~Coroutine()
	{
	}

	void Coroutine::resume()
	{
		LogDebug("in Coroutine::resume()");
		Context *pMainCtx = m_processor->getMainCtx();
		switch (m_status)
		{
		/*
			CO_READY 状态一般是针对新来的协程任务，所以还没有创建过上下文，需要先创建，再进行swap切换。
		*/
		case CO_READY:
			LogDebug("in Coroutine::resume() CO_READY");
			m_status = CO_RUNNING;
			m_ctx.makeContext((void (*)(void))coWrapFunc, m_processor, pMainCtx);
			m_ctx.swapToMe(pMainCtx);
			break;
		/*
			CO_WAITING 状态是之前被挂起的，挂起的时候协程内是保存了当时的上下文信息的，不需要重新makeContext，所以可以直接swap
		*/
		case CO_WAITING:
			m_status = CO_RUNNING;
			m_ctx.swapToMe(pMainCtx);
			break;

		default:
			break;
		}
	}

	void Coroutine::yield()
	{
		m_status = CO_WAITING;
	};

	void Coroutine::setProcessor(Processor *processor)
	{
		m_processor = processor;
	}
}