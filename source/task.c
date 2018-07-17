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


#if D_ENABLE_STACK_CHECK
void stack_overflow_check(T_tcb* tcb)
{
#if D_STACK_GROW_UP
	if (*(tcb->topstk + D_stk_offset) != D_task_lab) {
#else
	if (*(tcb->topstk - D_stk_offset) != D_task_lab) {
#endif
		D_print("task handl: %X->stack has oveflowed!\n", (DWORD)tcb);
		for (; ;) {
		}
	}
}
#endif

DWORD I_get_rttaskid(void)
{
	return (DWORD)g_rttask;
}

T_tcb* create_task_tcb(DWORD *addr, 
	INT16U prior)
{
	T_tcb* tcb;
	
	tcb = (T_tcb *)I_allocate(sizeof(T_tcb));
	D_assert(tcb)
	
	D_set_magic(tcb->flags, V_tcb_mn);
	tcb->tspaddr = addr;
	tcb->prior = prior;
	tcb->wkptick = 0;
	D_reset_tcbts(tcb->flags);
	
	tcb->last = NULL;
	tcb->next = NULL;
	tcb->stkaddr = NULL;
#if D_ENABLE_STACK_CHECK
	tcb->topstk = NULL;
#endif
#if D_ENABLE_TASK_STAT
	tcb->rtick = 0;
	tcb->rate = 0;
#endif
#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
	tcb->lhead = NULL;
#endif
#if D_ENABLE_MESSAGE_QUEUE
	tcb->pdata = NULL;
	tcb->size = 0;
#endif
#if D_ENABLE_SEMAPHORE
	D_reset_tcbwbad(tcb->flags);
	tcb->wprior = prior;
	tcb->windex = 0;
	tcb->wlist = NULL;
#endif
	return tcb;
}

T_tab* create_task_table(void)
{
	T_tab* tab;
	
	tab = (T_tab *)I_allocate(sizeof(T_tab));
	D_assert(tab)
	
	tab->lpre = NULL;
	tab->total = 0;
	tab->head = create_task_tcb(NULL, 0);
	D_assert(tab->head)
	
	tab->head->last = tab->head;
	tab->head->next = tab->head;
	return tab;
}

T_tcb* get_ready_task(void)
{
	T_tcb *tcb;
	T_tab *tab;
	
	tab = g_rdtab;
	if(tab->total) {
		if (tab->lpre->prior > D_get_maxp())
			tab->lpre = tab->head->next;
		
		tcb = tab->lpre;
		if (tab->lpre->next != tab->head)
			tab->lpre = tab->lpre->next;
		else
			tab->lpre = tab->head->next;
	}
	else
		tcb = g_IDLE;
	return tcb;
}

T_tab* get_task_table(T_tcb* tcb) 
{
	T_tab* tab = NULL;
	
	switch (D_get_tcbts(tcb->flags)) {
	case V_tcb_rd: {
		tab = g_rdtab;
		break;
	}
	case V_tcb_ws: {
		tab = g_wstab[0];
		break;
	}
	case V_tcb_wsof: {
		tab = g_wstab[1];
		break;
	}}
	return tab;
}

void move_task_to_table(T_tab *tab, 
	T_tcb *tcb) 
{
	T_tcb *temp;
	
	if (tab == g_rdtab) {
		temp = tab->head->next;
		while(temp != tab->head) {
			D_assert(temp)
			
			if (temp->prior > tcb->prior)
				break;
			temp = temp->next;
		}
	}
	else {
		if (tcb->wkptick == D_halt_tick)
			temp = tab->head;
		else {
			temp = tab->head->next;
			while(temp != tab->head) {
				D_assert(temp)
				
				if (temp->wkptick > tcb->wkptick)
					break;
				temp = temp->next;
			}
		}
	}

	tcb->last = temp->last;
	temp->last->next = tcb;
	tcb->next = temp;
	temp->last = tcb;
	if (tab->total) {
		if (tab == g_rdtab && tcb->prior < D_get_maxp()) {
			D_set_maxp(tcb->prior);
			D_set_tsd();
			tab->lpre = tab->head->next;
		}
		else if (tab->lpre != tab->head->next)
			tab->lpre = tab->head->next;
	}
	else {
		if (tab == g_rdtab) {
			D_set_maxp(tcb->prior);
			D_set_tsd();
		}
		tab->lpre = tcb;
	}
	
	if (tab == g_rdtab)
		D_set_tcbts(tcb->flags, V_tcb_rd)
	else if (tab == g_wstab[0])
		D_set_tcbts(tcb->flags, V_tcb_ws)
	else
		D_set_tcbts(tcb->flags, V_tcb_wsof)
	tab->total++;
}

void remove_task_from_table(T_tab *tab, 
	T_tcb *tcb)
{
	if (tcb == tab->lpre) {
		if (tab->total > 1) {
			if (tab == g_rdtab) {
				if (tcb->next == tab->head)
					tab->lpre = tab->head->next;
				else {
					if (tcb->next->prior > D_get_maxp()) {
						if (tcb->last != tab->head)
							tab->lpre = tab->head->next;
						else {
							D_set_maxp(tcb->next->prior);
							tab->lpre = tcb->next;
						}
					}
					else
						tab->lpre = tcb->next;
				}
			}
			else {
				if (tcb != tab->head->next)
					tab->lpre = tab->head->next;
				else
					tab->lpre = tcb->next;
			}
		}
		else {
			if (tab == g_rdtab)
				D_set_maxp(D_min_tprior)
			tab->lpre = NULL;
		}
	}

	tcb->last->next = tcb->next;
	tcb->next->last = tcb->last;
	tcb->last = NULL;
	tcb->next = NULL;
	
	D_reset_tcbts(tcb->flags);
	tab->total--;
}

void clean_overflowed_task_table() 
{
	T_tcb *tcb, *temp;
	T_tab* tab;

	tab = g_wstab[1];
	tcb = tab->head->next;
	while (tcb != tab->head) {
		D_assert(tcb)
		
		temp = tcb->next;
		remove_task_from_table(tab, tcb);
		tcb->wkptick = 0;
		move_task_to_table(g_wstab[0], tcb);
		tcb = temp;
	}
}

void I_task_exit(void) 
{
	if (!g_slock)
		task_scheduler(1, 1);
}

void I_drop_task(T_task_handl *handl) 
{
	DWORD xmask;
	T_tab* tab;
	T_tcb* tcb;
	
	if (!g_slock) {
		tcb = (T_tcb *)(*handl);
		if (tcb && D_get_magic(tcb->flags) == V_tcb_mn) {
			*handl = NULL;
			tab = get_task_table(tcb);
			D_assert(tab)
			if (tcb == g_rttask)
				task_scheduler(1, 1);
			else {
				D_enter_critical(xmask)
#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
	#if D_ENABLE_SEMAPHORE
				if (tab != g_rdtab && (tcb->lhead || tcb->wlist))
					clean_list_node(tcb);
	#else
				if (tab != g_rdtab && tcb->lhead)
					clean_list_node(tcb);
	#endif
#endif
				remove_task_from_table(tab, tcb);
				D_reset_magic(tcb->flags);
				I_release(tcb->stkaddr);
				I_release(tcb);
				D_leave_critical(xmask)
			}
		}
	}
}

void I_suspend_task(T_task_handl handl) 
{
	DWORD xmask;
	T_tab* tab;
	T_tcb* tcb;
	
	if (!g_slock) {
		tcb = (T_tcb *)handl;
		if (tcb && D_get_magic(tcb->flags) == V_tcb_mn) {
			D_enter_critical(xmask)
			tab = get_task_table(tcb);
			D_assert(tab)

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
	#if D_ENABLE_SEMAPHORE
			if (tab != g_rdtab && (tcb->lhead || tcb->wlist))
				clean_list_node(tcb);
	#else
			if (tab != g_rdtab && tcb->lhead)
				clean_list_node(tcb);
	#endif
#endif
			tcb->wkptick = D_halt_tick;
			remove_task_from_table(tab, tcb);
			move_task_to_table(g_wstab[0], tcb);
			D_leave_critical(xmask)
			if (tcb == g_rttask)
				task_scheduler(1, 0);
		}
	}
}

void I_restore_task(T_task_handl handl)
{
	DWORD xmask;
	T_tcb* tcb;
	T_tab* tab;
	
	tcb = (T_tcb *)handl;
	if (tcb && D_get_magic(tcb->flags) == V_tcb_mn && D_get_tcbts(tcb->flags) != V_tcb_rd) {
		D_enter_critical(xmask)
		tab = get_task_table(tcb);
		D_assert(tab)
		
		remove_task_from_table(tab, tcb);
		move_task_to_table(g_rdtab, tcb);
		D_leave_critical(xmask)
	}
}

void I_task_sleep(DWORD xms)
{
	DWORD xmask;
	T_tcb *tcb;
	T_tab* tab;
	
	D_assert(xms)
	if (!g_slock) {
		D_enter_critical(xmask)
		tcb = g_rttask;
		tcb->wkptick = get_next_tick(xms);
		if (tcb->wkptick < g_btick)
			tab = g_wstab[1];
		else
			tab = g_wstab[0];
		remove_task_from_table(g_rdtab, tcb);
		move_task_to_table(tab, tcb);
		D_leave_critical(xmask)
		task_scheduler(1, 0);
	}
}

T_tcb* create_core_task(INT16U prior,
	INT16U stksize,
	void (*pfunc)(void *uarg))
{
	DWORD xmask;
	DWORD *addr, *stk;
	T_tcb *tcb;
	
	addr = (DWORD *)I_allocate(sizeof(DWORD) * stksize);
	D_assert(addr)
	
	D_enter_critical(xmask)
#if D_STACK_GROW_UP
	stk = (addr + stksize - 1);
#else
	stk = addr;
#endif
	stk = task_stack_init(pfunc, NULL, stk, I_task_exit);
	tcb = create_task_tcb(stk, prior);
	D_assert(tcb)

	tcb->stkaddr = addr;
#if D_ENABLE_STACK_CHECK
	#if D_STACK_GROW_UP
		stk = addr;
		*(stk + D_stk_offset) = D_task_lab;
	#else
		stk = (addr + stksize - 1);
		*(stk - D_stk_offset) = D_task_lab;
	#endif
		tcb->topstk = stk;
#endif
	D_leave_critical(xmask)
	return tcb;
}

void I_create_task(T_task_handl* handl,
	INT16U prior, 
	INT16U stksize, 
	void (*pfunc)(void *uarg), 
	void* uarg) 
{
	DWORD xmask;
	DWORD *addr, *stk;
	T_tcb *tcb;
	
	D_assert(stksize)
	D_assert(pfunc)
	
	tcb = (T_tcb *)(*handl);
	if (D_get_magic(tcb->flags) != V_tcb_mn) {
		addr = (DWORD *)I_allocate(sizeof(DWORD) * stksize);
		D_assert(addr)
		
		D_enter_critical(xmask)
#if D_STACK_GROW_UP
		stk = (addr + stksize - 1);
#else
		stk = addr;
#endif
		stk = task_stack_init(pfunc, uarg, stk, I_task_exit);
		tcb = create_task_tcb(stk, prior);
		D_assert(tcb)
		
		tcb->stkaddr = addr;
#if D_ENABLE_STACK_CHECK
	#if D_STACK_GROW_UP
		stk = addr;
		*(stk + D_stk_offset) = D_task_lab;
	#else
		stk = (addr + stksize - 1);
		*(stk - D_stk_offset) = D_task_lab;
	#endif
		tcb->topstk = stk;
#endif
		move_task_to_table(g_rdtab, tcb);
		*handl = (T_task_handl)tcb;
		D_leave_critical(xmask)
	}
}
/* ----------------------------------- End of file --------------------------------------- */
