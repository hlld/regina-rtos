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


DWORD		g_ctrl;
DWORD		g_slock;
DWORD		g_btick;
DWORD		g_ltick;
DWORD		g_stick;
#if D_ENABLE_TASK_STAT
DWORD		g_ltmark;
#endif
T_tcb*		g_IDLE;
T_tcb*		g_TICK;
T_tcb*		g_temp;
T_tcb*		g_rdtask;
T_tcb*		g_rttask;
T_tab*		g_rdtab;
T_tab*		g_wstab[2];
#if D_ENABLE_SOFT_TIMER
T_ttab*		g_ttab[2];
#endif

DWORD I_get_rttick(void)
{
	return g_btick;
}

static void task_sleep_handl(void)
{
	DWORD xmask;
	T_tab *tab;
	T_tcb *tcb;
	
	if (D_test_isof()) {
		D_enter_critical(xmask)
		tab = g_wstab[0];
		g_wstab[0] = g_wstab[1];
		g_wstab[1] = tab;

		clean_overflowed_task_table();
		D_leave_critical(xmask)
	}

	tab = g_wstab[0];
	if (tab->total) {
		tcb = tab->lpre;
		if (g_btick > tcb->wkptick) {
			D_enter_critical(xmask)
			remove_task_from_table(tab, tcb);
			move_task_to_table(g_rdtab, tcb);

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
	#if D_ENABLE_SEMAPHORE
			if (tcb->lhead || tcb->wlist)
				clean_list_node(tcb);
	#else
			if (tcb->lhead)
				clean_list_node(tcb);
	#endif
#endif
			D_leave_critical(xmask)
		}
	}
}

void task_scheduler(INT8U isnext, 
	INT8U isexit)
{
	DWORD xmask;
	
	if (D_test_tsd()) {
		D_reset_tsd();
		isnext = TRUE;
	}
	D_enter_critical(xmask)
#if D_ENABLE_SWITCH_HOOK
	task_switch_out_hook(g_rttask);
#endif
#if D_ENABLE_STACK_CHECK
	stack_overflow_check(g_rttask);
#endif
#if D_ENABLE_TASK_STAT
	task_stat(g_rttask);
#endif
	if (isexit) {
		remove_task_from_table(g_rdtab, g_rttask);
		D_reset_magic(g_rttask->flags);
		I_release(g_rttask->stkaddr);
		I_release(g_rttask);
	}
	
	if (isnext) {
		g_rdtask = get_ready_task();
		g_stick = get_next_tick(D_TASK_TIME_SLICE);
	}
	else
		g_rdtask = g_temp;
	
#if D_ENABLE_SWITCH_HOOK
	task_switch_in_hook(g_rdtask);
#endif
#if D_ENABLE_TASK_STAT
	g_ltmark = g_btick;
#endif
	D_leave_critical(xmask)
	task_switch();
}

static void IDLE_task(void *arg) 
{
	for(; ;) {
#if D_ENABLE_IDLE_TASK_HOOK
		IDLE_task_hook();
#endif
	}
}

static void TICK_task(void *arg) 
{
	for(; ;) {
		D_tasks_lock()
		task_sleep_handl();
#if D_ENABLE_SOFT_TIMER
		timer_handl();
#endif
		if (D_test_isof()) {
			g_stick = 0;
			D_reset_isof();
		}
		
		if (g_btick > g_stick)
			D_set_tsd();

		if (g_btick < g_ltick)
			D_set_isof();

		g_ltick = g_btick;
		D_tasks_unlock()
		task_scheduler(0, 0);
	}
}

static void vars_init(void) 
{
	g_ctrl = 0;
	g_slock = 0;
	g_btick = 0;
	g_ltick = 0;
	g_stick = 0;
#if D_ENABLE_TASK_STAT
	g_ltmark = 0;
#endif
	g_IDLE = NULL;
	g_TICK = NULL;
	g_temp = NULL;
	g_rdtask = NULL;
	g_rttask = NULL;
	g_rdtab = NULL;
	g_wstab[0] = NULL;
	g_wstab[1] = NULL;
#if D_ENABLE_SOFT_TIMER
	g_ttab[0] = NULL;
	g_ttab[1] = NULL;
#endif
}

static void tasks_init(void) 
{
	g_IDLE = create_core_task(D_min_tprior,D_IDLE_STACK_SIZE, IDLE_task);
	D_assert(g_IDLE)
	
	g_TICK = create_core_task(D_max_tprior, D_TICK_STACK_SIZE, TICK_task);
	D_assert(g_TICK)
}

static void tabs_init(void) 
{
	g_rdtab = create_task_table();
	D_assert(g_rdtab)
	
	g_wstab[0] = create_task_table();
	D_assert(g_wstab[0])
	g_wstab[1] = create_task_table();
	D_assert(g_wstab[1])

#if D_ENABLE_SOFT_TIMER
	g_ttab[0] = create_timer_table();
	D_assert(g_ttab[0])
	g_ttab[1] = create_timer_table();
	D_assert(g_ttab[1])
#endif
}

void I_rtos_init(void) 
{
	heap_init();
	vars_init();
	tabs_init();
	tasks_init();
	tick_init();
}

void I_rtos_start(void)
{
	g_rdtask = g_IDLE;
	g_rttask = g_rdtask;
	g_temp = g_rdtask;
	g_stick = get_next_tick(D_TASK_TIME_SLICE);
	start_core();
}
/* ----------------------------------- End of file --------------------------------------- */
