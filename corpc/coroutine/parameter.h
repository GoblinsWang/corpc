/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_PARAMETER_H
#define CORPC_COROUTINE_PARAMETER_H
#include <stddef.h>

namespace corpc
{
	namespace parameter
	{
		// Stack size of coroutine
		const static size_t coroutineStackSize = 32 * 1024;

		// The initial length of the array of epoll_events
		static constexpr int epollerListFirstSize = 16;

		// The blocking duration of the epoll_wait
		static constexpr int epollTimeOutMs = 10000;

		// The length of the listening queue
		constexpr static unsigned backLog = 4096;

		// Request memPoolMallocObjCnt memory block of object size when there is no free memory block in the Memory pool
		static constexpr size_t memPoolMallocObjCnt = 40;
	}

}

#endif