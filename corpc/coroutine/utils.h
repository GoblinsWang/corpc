/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_UTILS_H
#define CORPC_COROUTINE_UTILS_H

#define DISALLOW_COPY_MOVE_AND_ASSIGN(TypeName) \
	TypeName(const TypeName &) = delete;        \
	TypeName(const TypeName &&) = delete;       \
	TypeName &operator=(const TypeName &) = delete

#endif
