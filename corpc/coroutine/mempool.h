/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#ifndef CORPC_COROUTINE_MEMPOOL_H
#define CORPC_COROUTINE_MEMPOOL_H

#include "parameter.h"
#include "utils.h"

namespace corpc
{
	struct MemBlockNode
	{
		union
		{
			MemBlockNode *next;
			char data;
		};
	};

	// Each time, you can get a memory block with the size of objSize from the Memory pool
	template <size_t objSize>
	class MemPool
	{
	public:
		MemPool()
			: m_freeListHead(nullptr), m_mallocListHead(nullptr), m_mallocTimes(0)
		{
			if (objSize < sizeof(MemBlockNode))
			{
				m_objSize = sizeof(MemBlockNode);
			}
			else
			{
				m_objSize = objSize;
			}
		};

		~MemPool();

		DISALLOW_COPY_MOVE_AND_ASSIGN(MemPool);

		void *AllocAMemBlock();
		void FreeAMemBlock(void *block);

	private:
		// free linked list
		MemBlockNode *m_freeListHead;

		// Large memory block linked list for malloc
		MemBlockNode *m_mallocListHead;

		// The number of actual mallocs
		size_t m_mallocTimes;

		// Size of each memory block
		size_t m_objSize;
	};

	template <size_t objSize>
	MemPool<objSize>::~MemPool()
	{
		while (m_mallocListHead)
		{
			MemBlockNode *mallocNode = m_mallocListHead;
			m_mallocListHead = mallocNode->next;
			free(static_cast<void *>(mallocNode));
		}
	}

	template <size_t objSize>
	void *MemPool<objSize>::AllocAMemBlock()
	{
		void *ret;
		if (nullptr == m_freeListHead)
		{
			size_t mallocCnt = parameter::memPoolMallocObjCnt + m_mallocTimes;
			void *newMallocBlk = malloc(mallocCnt * m_objSize + sizeof(MemBlockNode));
			MemBlockNode *mallocNode = static_cast<MemBlockNode *>(newMallocBlk);
			mallocNode->next = m_mallocListHead;
			m_mallocListHead = mallocNode;
			newMallocBlk = static_cast<char *>(newMallocBlk) + sizeof(MemBlockNode);
			for (size_t i = 0; i < mallocCnt; ++i)
			{
				MemBlockNode *newNode = static_cast<MemBlockNode *>(newMallocBlk);
				newNode->next = m_freeListHead;
				m_freeListHead = newNode;
				newMallocBlk = static_cast<char *>(newMallocBlk) + m_objSize;
			}
			++m_mallocTimes;
		}
		ret = &(m_freeListHead->data);
		m_freeListHead = m_freeListHead->next;
		return ret;
	}

	template <size_t objSize>
	void MemPool<objSize>::FreeAMemBlock(void *block)
	{
		if (nullptr == block)
		{
			return;
		}
		MemBlockNode *newNode = static_cast<MemBlockNode *>(block);
		newNode->next = m_freeListHead;
		m_freeListHead = newNode;
	}
}

#endif