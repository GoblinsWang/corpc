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

		// Use the function pointer to set the context entry of the current context
		void makeContext(void (*func)(), Processor *, Context *);

		// Directly use the current program state to set the context of the current context
		void makeCurContext();

		// Save the current context to oldCtx and switch to the current context
		void swapToMe(Context *pOldCtx);

		// Gets the coctx pointer for the current context
		inline struct coctx *getUCtx() { return &m_coctx; };

	private:
		// struct ucontext_t m_coctx;

		coctx m_coctx; // coroutine regs

		void *m_pStack;

		size_t m_stackSize;
	};

}

#endif