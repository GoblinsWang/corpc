/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <stdlib.h>
#include <string.h>
#include "context.h"
#include "parameter.h"
#include <stdlib.h>

namespace corpc
{
	extern "C"
	{
		// save current register's state to fitst coctx, and from second coctx take out register's state to assign register
		extern void coctx_swap(coctx *, coctx *) asm("coctx_swap");
	};

	Context::Context(size_t stackSize)
		: m_pStack(nullptr), m_stackSize(stackSize)
	{
	}

	Context::~Context()
	{
		if (m_pStack)
		{
			free(m_pStack);
		}
	}

	void Context::makeContext(void (*func)(), Processor *pP, Context *pLink)
	{
		if (nullptr == m_pStack)
		{
			m_pStack = malloc(m_stackSize);
		}
		// ::getcontext(&m_coctx);
		// m_coctx.uc_stack.ss_sp = m_pStack; // 分配一个新堆栈
		// m_coctx.uc_stack.ss_size = parameter::coroutineStackSize;

		// /*
		// 	定义一个后继上下文，传入的Context类型的plink上下文指针，一般为主线程的上下文信息
		// 	也就是说，当该协程执行完毕之后，就会切换回后继上下文。
		// */
		// m_coctx.uc_link = pLink->getUCtx();

		// /*
		// 	当上下文通过setcontext或者swapcontext激活后，执行func函数，argc为func的参数个数，后面是func的参数序列;
		// 	当func执行返回后，继承的上下文被激活，如果继承上下文为NULL时，线程退出。
		// */
		// makecontext(&m_coctx, func, 1, pP);

		void *top = m_pStack + m_stackSize;
		// first set 0 to stack
		// memset(&top, 0, m_stackSize);

		top = reinterpret_cast<char *>((reinterpret_cast<unsigned long>(top)) & -16LL);

		memset(&m_coctx, 0, sizeof(m_coctx));

		m_coctx.regs[kRSP] = top;
		m_coctx.regs[kRBP] = top;
		m_coctx.regs[kRETAddr] = reinterpret_cast<void *>(func);
		m_coctx.regs[kRDI] = reinterpret_cast<void *>(pP);
	}

	void Context::makeCurContext()
	{
		// ::getcontext(&m_coctx);
		memset(&m_coctx, 0, sizeof(m_coctx));
	}

	void Context::swapToMe(Context *pOldCtx)
	{
		// if (nullptr == pOldCtx)
		// {
		// 	setcontext(&m_coctx);
		// }
		// else
		// { /*
		// 	  将当前上下文保存到第一个上下文参数，载入第二个上下文参数
		//   */
		// 	swapcontext(pOldCtx->getUCtx(), &m_coctx);
		// }
		LogDebug("in Context::swapToMe()");
		coctx_swap(&(pOldCtx->m_coctx), &m_coctx);
		LogDebug("after coctx_swap()");
	}

}