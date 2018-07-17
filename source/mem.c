/**
 * ------------------------------------------------------------------------------------------
 * Regina V2.0 - Copyright (C) 2018 Hlld.
 * All rights reserved
 *
 * This file is part of Regina.
 *
 * Regina is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Regina is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Regina.  If not, see <http://www.gnu.org/licenses/>.
 * ------------------------------------------------------------------------------------------
 */

#include "regina.h"


#define D_heap_byte_align							0x08
#define D_heap_byte_align_mask						0x07
#define D_heap_min_block_size						(heap_struct_size << 1)

typedef struct lns
{
	DWORD 		size;
	struct 		lns* next;
} T_heap_lns;

#if D_ENABLE_EXTERNAL_HEAP
extern INT8U heap[D_HEAP_TOTAL_SIZE];
#else
static INT8U heap[D_HEAP_TOTAL_SIZE];
#endif

static T_heap_lns *lhead, *ltail;
static DWORD heap_struct_size;
DWORD heap_free_size;
DWORD lowest_heap_free_size;

DWORD I_get_free_heap_size(void)
{
	return heap_free_size;
}

DWORD I_get_free_heap_lrecord(void) 
{
	return lowest_heap_free_size;
}

#if D_ENABLE_HEAP_CHECK
static void heap_empty_check(void)
{
	if (heap_free_size < D_heap_min_block_size) {
		D_print("heap is nearly empty!\n");
		for (; ;) {
		}
	}
}
#endif

static void organize_free_heap(T_heap_lns* lt) 
{
	INT8U* t;
	T_heap_lns* list;

	for(list = lhead; list->next < lt; list = list->next) {
	}
	t = (INT8U *)list;
	if((t + list->size) == (INT8U *)lt) {
		list->size += lt->size;
		lt = list;
	}

	t = (INT8U *) lt;
	if((t + lt->size) == (INT8U *)list->next) {
		if(list->next != ltail) {
			lt->size += list->next->size;
			lt->next = list->next->next;
		} 
		else
			lt->next = ltail;
	}
	else
		lt->next = list->next;

	if(list != lt)
		list->next = lt;
}

void heap_init(void)
{
	INT8U* addr;
	DWORD xmask, heapaddr, totalsize;
	T_heap_lns* t;
	
	D_enter_critical(xmask)
	heap_struct_size = ((sizeof(T_heap_lns) + D_heap_byte_align - 1) & (~D_heap_byte_align_mask));
	heap_free_size = 0;
	lowest_heap_free_size = 0;

	heapaddr = (DWORD)heap;
	totalsize = D_HEAP_TOTAL_SIZE;
	if ((heapaddr & D_heap_byte_align_mask)) {
		heapaddr = ((heapaddr + D_heap_byte_align - 1) & (~D_heap_byte_align_mask));
		totalsize -= heapaddr - (DWORD)heap;
	}
	
	addr = (INT8U *)heapaddr;
	lhead = (T_heap_lns *)heapaddr;
	heapaddr += heap_struct_size;
	lhead->size = 0;
	lhead->next = (T_heap_lns *)heapaddr;
	heapaddr += (totalsize - (heap_struct_size << 1));
	heapaddr &= ~(D_heap_byte_align_mask);
	
	ltail = (T_heap_lns*)heapaddr;
	ltail->size = 0;
	ltail->next = NULL;

	t = lhead->next;
	t->next = ltail;
	t->size = (heapaddr - (DWORD)addr - heap_struct_size);
	heap_free_size = t->size;
	lowest_heap_free_size = t->size;
	D_leave_critical(xmask)
}

void I_release(void* ptr)
{
	T_heap_lns* p;
	INT8U* t;
	DWORD xmask;
	
	D_enter_critical(xmask)
	D_assert(ptr)
	t = (INT8U *)ptr;
	t -= heap_struct_size;
	p = (T_heap_lns *)t;
	
	D_assert(p->size & D_heap_lab)
	D_assert(!p->next)
	if (p->size & D_heap_lab && !p->next) {
		p->size &= ~D_heap_lab;
		heap_free_size += p->size;
		organize_free_heap(p);
	}
	D_leave_critical(xmask)
}

void* I_allocate(DWORD size)
{
	void* ptr = NULL;
	T_heap_lns* lastl, *pl, *newl;
	DWORD xmask;
	
	D_enter_critical(xmask)
	D_assert(size)
	
	size += heap_struct_size;
	if (size & D_heap_byte_align_mask) {
		size += (D_heap_byte_align - (size & D_heap_byte_align_mask));
		D_assert(!(size & D_heap_byte_align_mask))
	}

	if (size <= heap_free_size) {
		lastl = lhead;
		pl = lhead->next;

		while((pl->size < size) && pl->next) {
			lastl = pl;
			pl = pl->next;
		}

		if(pl != ltail) {
			ptr = (void *)((DWORD)(lastl->next) + heap_struct_size);
			D_assert(!((DWORD)ptr & D_heap_byte_align_mask))

			lastl->next = pl->next;
			if((pl->size - size) > D_heap_min_block_size) {
				newl = (T_heap_lns *)((DWORD)pl + size);
				D_assert(!((DWORD)newl & D_heap_byte_align_mask))

				newl->size = pl->size - size;
				pl->size = size;
				organize_free_heap(newl);
			}

			heap_free_size -= pl->size;
			if(heap_free_size < lowest_heap_free_size)
				lowest_heap_free_size = heap_free_size;
			pl->size |= D_heap_lab;
			pl->next = NULL;
		}
	}
#if D_ENABLE_HEAP_CHECK
	heap_empty_check();
#endif
	D_leave_critical(xmask)
	return ptr;
}
/* ----------------------------------- End of file --------------------------------------- */
