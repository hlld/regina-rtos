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


#if D_ENABLE_SEMAPHORE
void I_create_sem(T_sem_handl* handl,
	INT8U type,
	DWORD initv) 
{
	DWORD xmask;
	T_sem* sem;
	
	sem = (T_sem *)(*handl);
	if (D_get_magic(sem->flags) != V_sem_mn) {
		sem = (T_sem *)I_allocate(sizeof(T_sem));
		D_assert(sem)
		D_enter_critical(xmask)
		
		D_set_magic(sem->flags, V_sem_mn)
		D_reset_semlock(sem->flags);
		D_set_semisrxe(sem->flags);
		sem->holder = NULL;
		sem->value = 0;
		
		sem->wrxh = create_list_node(NULL);
		D_assert(sem->wrxh)
		sem->wrxh->last = sem->wrxh;
		sem->wrxh->next = sem->wrxh;

		switch (type) {
		case V_sem_binary: {
			D_set_semtype(sem->flags, V_sem_binary);
			if (initv)
				sem->value = 1;
			break;
		}
		case V_sem_count: {
			D_set_semtype(sem->flags, V_sem_count);
			sem->value = initv;
			break;
		}
		case V_sem_mutex: {
			D_set_semtype(sem->flags, V_sem_mutex);
			break;
		}
		case V_sem_remutex: {
			D_set_semtype(sem->flags, V_sem_remutex);
			break;
		}
		default: {
			D_set_semtype(sem->flags, V_sem_binary);
			if (initv)
				sem->value = 1;
			break;
		}}
		*handl = (T_sem_handl)sem;
		D_leave_critical(xmask)
	}
}

static INT8U get_index_bit(T_sem_handl* list, T_sem_handl handl)
{
	INT8U index, result;
	
	index = 0x00;
	result = 0xff;
	while (list[index]) {
		if (list[index] == handl) {
			result = index;
			break;
		}
		index++;
	}
	return result;
}

static void clean_list(T_sem* sem) 
{
	INT8U index;
	T_tcb* tcb;
	T_list* list, *t;
	
	list = sem->wrxh->next;
	while(list != sem->wrxh) {
		D_assert(list)
		
		t = list->next;
		remove_node_from_list(list);
		
		tcb = list->ptcb;
		tcb->lhead = NULL;
		if (tcb->wlist) {
			index = get_index_bit(tcb->wlist, (T_sem_handl *)sem);
			D_assert(index < 32)
			D_reset_bno(tcb->windex, index);
			D_set_tcbwbad(tcb->flags);
		}
		I_release(list);
		list = t;
	}
}

void I_drop_sem(T_sem_handl *handl) 
{
	DWORD xmask;
	T_sem* sem;
	
	sem = (T_sem *)(*handl);
	if (sem && D_get_magic(sem->flags) == V_sem_mn) {
		D_enter_critical(xmask)
		D_set_semlock(sem->flags);
		D_reset_magic(sem->flags);
		
		clean_list(sem);
		I_release(sem->wrxh);
		I_release(sem);
		*handl = NULL;
		D_leave_critical(xmask)
	}
}

void I_lock_sem(T_sem_handl handl) 
{
	T_sem* sem;
	
	sem = (T_sem *)handl;
	if (sem && D_get_magic(sem->flags) == V_sem_mn) {
		D_set_semlock(sem->flags);
	}
}

void I_unlock_sem(T_sem_handl handl) 
{
	T_sem* sem;
	
	sem = (T_sem *)handl;
	if (sem && D_get_magic(sem->flags) == V_sem_mn) {
		D_reset_semlock(sem->flags);
	}
}

static void remove_wrxtask_form_sem(T_sem* sem)
{
	INT8U index;
	T_tcb* tcb;
	T_list* list;
	T_tab* tab;
	
	list = sem->wrxh->next;
	tcb = list->ptcb;
	switch (D_get_semtype(sem->flags)) {
	case V_sem_mutex:
	case V_sem_remutex: {
		sem->holder = tcb;
		break;
	}}
	remove_node_from_list(list);
	I_release(list);
	if (TRUE == test_list_empty(sem->wrxh))
		D_set_semisrxe(sem->flags);
	
	if (tcb->wlist) {
		index = get_index_bit(tcb->wlist, sem);
		D_assert(index < 32)
		D_reset_bno(tcb->windex, index);
		if (!tcb->windex && !D_test_tcbwbad(tcb->flags))
			D_set_tcbwr(tcb->flags);
	}
	else
		D_set_tcbwr(tcb->flags);

	if (D_test_tcbwr(tcb->flags)) {
		tab = get_task_table(tcb);
		D_assert(tab)
		
		remove_task_from_table(tab, tcb);
		move_task_to_table(g_rdtab, tcb);
	}
}

static void add_list_node(T_sem* sem,
	DWORD afterms)
{
	T_tcb* tcb;
	T_list* list;
	
	tcb = g_rttask;
	if (!g_slock && afterms) {
		list = create_list_node(tcb);
		D_assert(list)
		
		if (!tcb->wlist)
			tcb->lhead = sem->wrxh;
		move_node_to_list(sem->wrxh, list);
		D_reset_semisrxe(sem->flags);
	}
}

static void move_task_to_wlist(DWORD afterms)
{
	DWORD xmask;
	T_tcb* tcb;
	T_tab* tab;

	if (!g_slock && afterms) {
		tcb = g_rttask;
		D_enter_critical(xmask)
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

static void sort_list_node(T_list* head, 
	T_tcb* tcb)
{
	T_list* list;
	
	list = get_list_node(head, tcb);
	if (list) {
		remove_node_from_list(list);
		move_node_to_list(head, list);
	}
}

static void adjust_task_prior(T_tcb *tcb, 
	INT16U newprior)
{
	INT8U index;

	tcb->prior = newprior;
	switch (D_get_tcbts(tcb->flags)) {
	case V_tcb_ws:
	case V_tcb_wsof: {
		if (tcb->wlist) {
			index = 0;
			while (tcb->wlist[index]) {
				if (D_test_bno(tcb->windex, index))
					sort_list_node(((T_sem *)tcb->wlist[index])->wrxh, tcb);
				index++;
			}
		}
		else if (tcb->lhead) {
			sort_list_node((T_list *)tcb->lhead, tcb);
		}
		break;
	}
	case V_tcb_rd: {
		remove_task_from_table(g_rdtab, tcb);
		move_task_to_table(g_rdtab, tcb);
		break;
	}}
}

RESULT I_post_sem(T_sem_handl handl)
{
	DWORD xmask;
	T_sem* sem;
	T_tcb* tcb;
	
	tcb = g_rttask;
	D_reset_tcbwr(tcb->flags);
	sem = (T_sem *)handl;
	if (sem && D_get_magic(sem->flags) == V_sem_mn && !D_test_semlock(sem->flags)) {
		switch (D_get_semtype(sem->flags)) {
		case V_sem_binary:
		case V_sem_count: {
			D_enter_critical(xmask)
			if (D_get_semtype(sem->flags) == V_sem_count) {
				sem->value++;
				D_set_tcbwr(tcb->flags);
			}
			else if (!sem->value) {
				sem->value = 1;
				D_set_tcbwr(tcb->flags);
			}
			if (!D_test_semisrxe(sem->flags)) {
				sem->value--;
				remove_wrxtask_form_sem(sem);
			}
			D_leave_critical(xmask)
			break;
		}
		case V_sem_mutex:
		case V_sem_remutex: {
			if (tcb == sem->holder && sem->value) {
				D_enter_critical(xmask)
				sem->value--;
				D_set_tcbwr(tcb->flags);
				if (!sem->value) {
					if (!D_test_semisrxe(sem->flags)) {
						sem->value++;
						remove_wrxtask_form_sem(sem);
					}
					if (tcb->prior < tcb->wprior)
						adjust_task_prior(tcb, tcb->wprior);
				}
				D_leave_critical(xmask)
			}
			break;
		}}
	}
	
	if (D_test_tcbwr(tcb->flags))
		return TRUE;
	else
		return FALSE;
}

RESULT I_post_semisr(T_sem_handl handl) 
{
	RESULT result = FALSE;
	DWORD xmask;
	T_sem* sem;
	
	sem = (T_sem *)handl;
	if (sem && D_get_magic(sem->flags) == V_sem_mn && !D_test_semlock(sem->flags)) {
		switch (D_get_semtype(sem->flags)) {
		case V_sem_binary:
		case V_sem_count: {
			D_enter_critical(xmask)
			if (D_get_semtype(sem->flags) == V_sem_count) {
				sem->value++;
				result = TRUE;
			}
			else if (!sem->value) {
				sem->value = 1;
				result = TRUE;
			}
			if (!D_test_semisrxe(sem->flags)) {
				sem->value--;
				remove_wrxtask_form_sem(sem);
			}
			D_leave_critical(xmask)
			break;
		}}
	}
	return result;
}

RESULT I_wait_for_sem(T_sem_handl handl, 
	DWORD afterms) 
{
	INT8U iswait = FALSE;
	DWORD xmask;
	T_sem* sem;
	T_tcb* tcb;
	
	tcb = g_rttask;
	D_reset_tcbwr(tcb->flags);
	sem = (T_sem *)handl;
	if (sem && D_get_magic(sem->flags) == V_sem_mn && !D_test_semlock(sem->flags)) {
		switch (D_get_semtype(sem->flags)) {
		case V_sem_binary:
		case V_sem_count: {
			D_enter_critical(xmask)
			if (sem->value) {
				sem->value--;
				D_set_tcbwr(tcb->flags);
			}
			else {
				add_list_node(sem, afterms);
				iswait = TRUE;
			}
			D_leave_critical(xmask)
			break;
		}
		case V_sem_remutex: {
			if (tcb == sem->holder) {
				sem->value++;
				D_set_tcbwr(tcb->flags);
				break;
			}
		}
		case V_sem_mutex: {
			D_enter_critical(xmask)
			if (!sem->value) {
				sem->value++;
				sem->holder = tcb;
				D_set_tcbwr(tcb->flags);
			}
			else {
				if (tcb->prior < sem->holder->prior)
					adjust_task_prior(sem->holder, tcb->prior);
				add_list_node(sem, afterms);
				iswait = TRUE;
			}
			D_leave_critical(xmask)
			break;
		}}
		if (iswait)
			move_task_to_wlist(afterms);
	}
	
	tcb->lhead = NULL;
	if (D_test_tcbwr(tcb->flags))
		return TRUE;
	else
		return FALSE;
}

RESULT I_wait_for_mults(T_sem_handl* handl, 
	DWORD afterms) 
{
	INT8U index = 0, ntag = 0;
	DWORD xmask;
	T_sem* sem;
	T_tcb* tcb;
	
	D_assert(handl)
	tcb = g_rttask;
	D_reset_tcbwr(tcb->flags);
	tcb->wlist = handl;
	while (handl[ntag]) {
		sem = (T_sem *)handl[ntag];
		if (D_get_magic(sem->flags) == V_sem_mn && !D_test_semlock(sem->flags)) {
			ntag++;
			if (ntag >= 32)
				break;
		}
		else {
			ntag = 0;
			break;
		}
	}
	if (ntag) {
		D_index_init(tcb->windex, ntag);
		while (index < ntag) {
			sem = (T_sem *)handl[index];
			switch (D_get_semtype(sem->flags)) {
			case V_sem_binary:
			case V_sem_count: {
				D_enter_critical(xmask)
				if (sem->value) {
					sem->value--;
					D_reset_bno(tcb->windex, index);
				}
				else
					add_list_node(sem, afterms);
				D_leave_critical(xmask)
				break;
			}
			case V_sem_remutex: {
				if (sem->holder == tcb) {
					sem->value++;
					D_reset_bno(tcb->windex, index);
					break;
				}
			}
			case V_sem_mutex: {
				D_enter_critical(xmask)
				if (!sem->value) {
					sem->value++;
					sem->holder = tcb;
					D_reset_bno(tcb->windex, index);
				}
				else {
					if (tcb->prior < sem->holder->prior)
						adjust_task_prior(sem->holder, tcb->prior);
					add_list_node(sem, afterms);
				}
				D_leave_critical(xmask)
				break;
			}}
			index++;
		}
		if (tcb->windex || D_test_tcbwbad(tcb->flags))
			move_task_to_wlist(afterms);
		else
			D_set_tcbwr(tcb->flags);
		if (!D_test_tcbwr(tcb->flags)) {
			index = 0;
			while (handl[index]) {
				if (!D_test_bno(tcb->windex, index))
					I_post_sem(handl[index]);
				index++;
			}
			D_reset_tcbwr(tcb->flags);
		}
	}

	tcb->wlist = NULL;
	tcb->lhead = NULL;
	if (D_test_tcbwr(tcb->flags))
		return TRUE;
	else
		return FALSE;
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
