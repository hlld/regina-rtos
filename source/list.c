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


#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
T_list* create_list_node(T_tcb* tcb) 
{
	T_list* list;
	
	list = (T_list *)I_allocate(sizeof(T_list));
	D_assert(list)
	
	list->ptcb = tcb;
	list->last = NULL;
	list->next = NULL;
	return list;
}

void move_node_to_list(T_list *head, 
	T_list *list) 
{
	T_list *t;

	t = head->next;
	while(t != head) {
		D_assert(t)

		if (t->ptcb->prior > list->ptcb->prior)
			break;
		t = t->next;
	}

	list->last = t->last;
	t->last->next = list;
	list->next = t;
	t->last = list;
}

RESULT test_list_empty(T_list *head)
{
	if (head->next == head && head->last == head)
		return TRUE;
	return FALSE;
}

void remove_node_from_list(T_list *list) 
{
	list->last->next = list->next;
	list->next->last = list->last;
	list->last = NULL;
	list->next = NULL;
}

T_list* get_list_node(T_list *head, 
	T_tcb *tcb) 
{
	T_list* list;
	
	list = head->next;
	while(list != head) {
		D_assert(list)
		
		if (list->ptcb == tcb)
			break;
		list = list->next;
	}
	if (list == head)
		list = NULL;
	return list;
}

static void drop_list_node(T_list* head, 
	T_tcb* tcb) 
{
	T_list* list;

	list = get_list_node(head, tcb);
	if (list) {
		remove_node_from_list(list);
		I_release(list);
	}
}

void clean_list_node(T_tcb* tcb) 
{
#if D_ENABLE_SEMAPHORE
	INT8U index;

	if (tcb->wlist) {
		index = 0;
		while (tcb->wlist[index]) {
			if (D_test_bno(tcb->windex, index))
				drop_list_node(((T_sem *)tcb->wlist[index])->wrxh, tcb);
			index++;
		}
	}
	else
		drop_list_node(tcb->lhead, tcb);
#else
	drop_list_node(tcb->lhead, tcb);
#endif
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
