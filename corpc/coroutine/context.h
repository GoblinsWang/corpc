/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_CONTEXT_H
#define CORPC_COROUTINE_CONTEXT_H
#include "utils.h"
#include "parameter.h"
#include "../log/logger.h"
// #include <ucontext.h>

namespace corpc
{

	enum
	{
		kRBP = 6,	  // rbp, bottom of stack
		kRDI = 7,	  // rdi, first para when call function
		kRSI = 8,	  // rsi, second para when call function
		kRETAddr = 9, // the next excute cmd address, it will be assigned to rip
		kRSP = 13,	  // rsp, top of stack
	};

	struct coctx
	{
		void *regs[14];
	};

	class Processor;
	class Context
	{
	public:
		Context(size_t stackSize);
		~Context();

		Context(const Context &otherCtx)
			: m_coctx(otherCtx.m_coctx), m_pStack(otherCtx.m_pStack)
		{
		}

		Context(Context &&otherCtx)
			: m_coctx(otherCtx.m_coctx), m_pStack(otherCtx.m_pStack)
		{
		}

		Context &operator=(const Context &otherCtx) = delete;

		// 用函数指针设置当前context的上下文入口
		void makeContext(void (*func)(), Processor *, Context *);

		// 直接用当前程序状态设置当前context的上下文
		void makeCurContext();

		// 将当前上下文保存到oldCtx中，然后切换到当前上下文，若oldCtx为空，则直接运行
		void swapToMe(Context *pOldCtx);

		// 获取当前上下文的ucontext_t指针
		// inline struct ucontext_t *getUCtx() { return &m_coctx; };
		inline struct coctx *getUCtx() { return &m_coctx; };

	private:
		// struct ucontext_t m_coctx;

		coctx m_coctx; // coroutine regs

		void *m_pStack;

		size_t m_stackSize;
	};

}

#endif