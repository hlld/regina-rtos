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


#if D_ENABLE_MESSAGE_QUEUE
void* I_get_msg_data(void) {
	return g_rttask->pdata;
}

DWORD I_get_msg_size(void) {
	return g_rttask->size;
}

void I_create_msgq(T_msgq_handl *handl) 
{
	DWORD xmask;
	T_msgq* q;
	
	q = (T_msgq *)(*handl);
	if (D_get_magic(q->flags) != V_msg_mn) {
		q = (T_msgq *)I_allocate(sizeof(T_msgq));
		D_assert(q)
		D_enter_critical(xmask)
		
		D_set_magic(q->flags, V_msg_mn);
		D_reset_msglock(q->flags);
		D_set_msgisqe(q->flags);
		D_reset_msgisqf(q->flags);
		D_set_msgisrxe(q->flags);
		D_set_msgistxe(q->flags);
		
		q->queue = create_msg_queue();
		D_assert(q->queue)

		q->wrxh = create_list_node(NULL);
		D_assert(q->wrxh)
		q->wrxh->next = q->wrxh;
		q->wrxh->last = q->wrxh;
		
		q->wtxh = create_list_node(NULL);
		D_assert(q->wtxh)
		q->wtxh->next = q->wtxh;
		q->wtxh->last = q->wtxh;
		
		*handl = (T_msgq_handl)q;
		D_leave_critical(xmask)
	}
}

static void clean_list(T_list* head) 
{
	T_list* list, *t;

	list = head->next;
	while(list != head) {
		D_assert(list)
		
		t = list->next;
		remove_node_from_list(list);
		list->ptcb->lhead = NULL;
		I_release(list);
		list = t;
	}
}

void I_drop_msgq(T_msgq_handl *handl) 
{
	DWORD xmask;
	T_msgq* q;
	
	q = (T_msgq *)(*handl);
	if (q && D_get_magic(q->flags) == V_msg_mn) {
		D_enter_critical(xmask)
		
		D_set_msglock(q->flags);
		D_reset_magic(q->flags);
		
		clean_list(q->wrxh);
		clean_list(q->wtxh);
		
		I_release(q->queue);
		I_release(q->wrxh);
		I_release(q->wtxh);
		I_release(q);
		*handl = NULL;
		D_leave_critical(xmask)
	}
}

void I_lock_msgq(T_msgq_handl handl)
{
	T_msgq* q;
	
	q = (T_msgq *)handl;
	if (q && D_get_magic(q->flags) == V_msg_mn)
		D_set_msglock(q->flags);
}

void I_unlock_msgq(T_msgq_handl handl) 
{
	T_msgq* q;

	q = (T_msgq *)handl;
	if (q && D_get_magic(q->flags) == V_msg_mn)
		D_reset_msglock(q->flags);
}

static void wakeup_task(T_tcb* tcb) 
{
	T_tab* tab;
	
	tab = get_task_table(tcb);
	D_assert(tab)
	
	remove_task_from_table(tab, tcb);
	move_task_to_table(g_rdtab, tcb);
}

static void move_msg_to_queue(T_msgq* q, T_msg* msg)
{
	if (FALSE == move_element_to_queue(q->queue, msg))
		D_set_msgisqf(q->flags);
	else if (TRUE == test_queue_full(q->queue))
		D_set_msgisqf(q->flags);
	D_reset_msgisqe(q->flags);
}

static void remove_msg_from_queue(T_msgq* q, T_msg* msg)
{
	if (FALSE == remove_element_from_queue(q->queue, msg))
		D_set_msgisqe(q->flags);
	else if (TRUE == test_queue_empty(q->queue))
		D_set_msgisqe(q->flags);
	D_reset_msgisqf(q->flags);
}

static void remove_wrxtask_form_msgq(T_msgq* q) 
{
	T_tcb* tcb;
	T_list* list;
	T_msg msg;
	
	list = q->wrxh->next;
	tcb = list->ptcb;
	remove_node_from_list(list);
	I_release(list);
	if (TRUE == test_list_empty(q->wrxh))
		D_set_msgisrxe(q->flags);
	
	if (D_test_tcbisf(tcb->flags))
		remove_msg_from_queue(q, &msg);
	else
		read_first_element(q->queue, &msg);
	tcb->pdata = msg.pdata;
	tcb->size = msg.size;

	D_set_tcbwr(tcb->flags);
	wakeup_task(tcb);
}

static void remove_wtxtask_from_msgq(T_msgq* q) 
{
	T_tcb* tcb;
	T_list* list;
	T_msg msg;
	
	list = q->wtxh->next;
	tcb = list->ptcb;
	remove_node_from_list(list);
	I_release(list);
	if (TRUE == test_list_empty(q->wtxh))
		D_set_msgistxe(q->flags);
	
	msg.pdata = tcb->pdata;
	msg.size = tcb->size;
	move_msg_to_queue(q, &msg);
	
	D_set_tcbwr(tcb->flags);
	wakeup_task(tcb);
}

static void move_task_to_wlist(T_msgq* q, 
	DWORD afterms, 
	INT8U option) 
{
	DWORD xmask;
	T_tcb* tcb;
	T_list* list, *t;
	T_tab* tab;

	if (!g_slock && afterms) {
		tcb = g_rttask;
		if (option) {
			t = q->wrxh;
			D_reset_msgisrxe(q->flags);
		}
		else {
			t = q->wtxh;
			D_reset_msgistxe(q->flags);
		}
		
		list = create_list_node(tcb);
		D_assert(list)
		D_enter_critical(xmask)
		tcb->lhead = t;
		move_node_to_list(t, list);
		
		tab = g_wstab[0];
		if (afterms == D_halt_tick)
			tcb->wkptick = D_halt_tick;
		else {
			tcb->wkptick = get_next_tick(afterms);
			if (tcb->wkptick < g_btick)
				tab = g_wstab[1];
		}
		
		remove_task_from_table(g_rdtab, tcb);
		move_task_to_table(tab, tcb);
		D_leave_critical(xmask)
		task_scheduler(1, 0);
	}
}

RESULT I_send_msg(T_msgq_handl handl, 
	void *pdata, 
	DWORD size, 
	DWORD afterms)
{
	DWORD xmask;
	T_tcb* tcb;
	T_msg msg;
	T_msgq* q;
	
	tcb = g_rttask;
	D_reset_tcbwr(tcb->flags);
	q = (T_msgq *)handl;
	if (q && D_get_magic(q->flags) == V_msg_mn && !D_test_msglock(q->flags)) {
		if (!D_test_msgisqf(q->flags)) {
			msg.pdata = pdata;
			msg.size = size;

			D_enter_critical(xmask)
			move_msg_to_queue(q, &msg);
			if (!D_test_msgisrxe(q->flags))
				remove_wrxtask_form_msgq(q);
			D_set_tcbwr(tcb->flags);
			D_leave_critical(xmask)
		}
		else {
			tcb->pdata = pdata;
			tcb->size = size;
			move_task_to_wlist(q, afterms, 0);
		}
	}

	tcb->lhead = NULL;
	if (D_test_tcbwr(tcb->flags))
		return TRUE;
	else
		return FALSE;
}

RESULT I_send_msgisr(T_msgq_handl handl, 
	void *pdata, 
	DWORD size) 
{
	RESULT result = FALSE;
	DWORD xmask;
	T_msg msg;
	T_msgq* q;
	
	q = (T_msgq *)handl;
	if (q && D_get_magic(q->flags) == V_msg_mn && !D_test_msglock(q->flags) &&
			!D_test_msgisqf(q->flags)) {
		D_enter_critical(xmask)
		msg.pdata = pdata;
		msg.size = size;
		move_msg_to_queue(q, &msg);

		if (!D_test_msgisrxe(q->flags))
			remove_wrxtask_form_msgq(q);
		result = TRUE;
		D_leave_critical(xmask)
	}
	return result;
}

RESULT I_wait_for_msg(T_msgq_handl handl, 
	INT8U isfetch, 
	DWORD afterms)
{
	DWORD xmask;
	T_tcb* tcb;
	T_msg msg;
	T_msgq* q;
	
	tcb = g_rttask;
	if (isfetch)
		D_set_tcbisf(tcb->flags);
	else
		D_reset_tcbisf(tcb->flags);
	D_reset_tcbwr(tcb->flags);
	
	q = (T_msgq *)handl;
	if (q && D_get_magic(q->flags) == V_msg_mn && !D_test_msglock(q->flags)) {
		if (!D_test_msgisqe(q->flags)) {
			D_enter_critical(xmask)

			if (isfetch)
				remove_msg_from_queue(q, &msg);
			else
				read_first_element(q->queue, &msg);
			tcb->pdata = msg.pdata;
			tcb->size = msg.size;
			D_set_tcbwr(tcb->flags);
			
			if (!D_test_msgistxe(q->flags))
				remove_wtxtask_from_msgq(q);
			D_leave_critical(xmask)
		}
		else
			move_task_to_wlist(q, afterms, 1);
	}
	
	tcb->lhead = NULL;
	if (D_test_tcbwr(tcb->flags))
		return TRUE;
	else
		return FALSE;
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
