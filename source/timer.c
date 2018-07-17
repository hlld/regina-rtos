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


#if D_ENABLE_SOFT_TIMER
static T_tim* create_timer_node(INT8U isperiodic, 
	DWORD period,
	void (*pfunc)(void *uarg),
	void *uarg)
{
	T_tim* t;
	
	t = (T_tim *)I_allocate(sizeof(T_tim));
	D_assert(t)
	
	D_set_magic(t->flags, V_tim_mn);
	D_reset_timisof(t->flags);
	
	t->atick = 0;
	t->uarg = uarg;
	t->last = NULL;
	t->next = NULL;
	
	t->period = period;
	t->pfunc = pfunc;
	if (isperiodic)
		D_set_timisp(t->flags);
	else
		D_reset_timisp(t->flags);
	return t;
}

T_ttab* create_timer_table(void)
{
	T_ttab* tt;
	
	tt = (T_ttab *)I_allocate(sizeof(T_ttab));
	D_assert(tt)

	tt->total = 0;
	tt->head = create_timer_node(0, 0, NULL, NULL);
	D_assert(tt->head)
	
	tt->head->last = tt->head;
	tt->head->next = tt->head;
	return tt;
}

static void move_timer_to_table(T_ttab* tt, 
	T_tim *t) 
{
	T_tim *temp;
	
	if (t->atick == D_halt_tick)
		temp = tt->head;
	else {
		temp = tt->head->next;
		
		while(temp != tt->head) {
			D_assert(temp)
			
			if (temp->atick > t->atick)
				break;
			temp = temp->next;
		}
	}

	t->last = temp->last;
	temp->last->next = t;
	t->next = temp;
	temp->last = t;	
	
	if (tt == g_ttab[0])
		D_set_timisof(t->flags, V_tim_nof)
	else
		D_set_timisof(t->flags, V_tim_of)
	tt->total++;
}

void remove_timer_from_table(T_ttab* tt, T_tim* t)
{
	t->last->next = t->next;
	t->next->last = t->last;
	t->last = NULL;
	t->next = NULL;
	
	D_reset_timisof(t->flags);
	tt->total--;
}

T_ttab* get_timer_table(T_tim* t)
{
	T_ttab* tt = NULL;
	
	switch (D_get_timisof(t->flags)) {
	case V_tim_nof: {
		tt = g_ttab[0];
		break;
	}
	case V_tim_of: {
		tt = g_ttab[1];
		break;
	}}
	return tt;
}

static void clean_overflowed_timer_table(void) 
{
	T_tim* t, *temp;
	T_ttab* tt;
	
	tt = g_ttab[1];
	t = tt->head->next;
	while (t != tt->head) {
		D_assert(t)
		
		temp = t->next;
		remove_timer_from_table(tt, t);
		t->atick = 0;
		move_timer_to_table(g_ttab[0], t);
		t = temp;
	}
}

void I_set_timer_arg(T_tim_handl handl, 
	void * uarg) 
{
	T_tim* t;
	
	t = (T_tim *)handl;
	if (t && D_get_magic(t->flags) == V_tim_mn)
		t->uarg = uarg;
}

void I_stop_timer(T_tim_handl handl) 
{
	T_tim* t;
	T_ttab* tt;

	t = (T_tim *)handl;
	if (t && D_get_magic(t->flags) == V_tim_mn) {
		D_tasks_lock()
		tt = get_timer_table(t);
		D_assert(tt)
		
		remove_timer_from_table(tt, t);
		t->atick = D_halt_tick;
		move_timer_to_table(tt, t);
		D_tasks_unlock()
	}
}

void I_start_timer(T_tim_handl handl) 
{
	INT8U label;
	T_tim* t;
	T_ttab* tt;

	t = (T_tim *)handl;
	if (t && D_get_magic(t->flags) == V_tim_mn) {
		D_tasks_lock()
		if (t->atick != D_halt_tick) {
			if (!D_test_timisp(t->flags))
				label = TRUE;
		}
		else
			label = TRUE;
		
		if (TRUE == label) {
			tt = get_timer_table(t);
			D_assert(tt)
			
			remove_timer_from_table(tt, t);
			t->atick = get_next_tick(t->period);
			if (t->atick < g_btick)
				tt = g_ttab[1];
			else
				tt = g_ttab[0];
			move_timer_to_table(tt, t);
		}
		D_tasks_unlock()
	}
}

void I_drop_timer(T_tim_handl *handl) 
{
	DWORD xmask;
	T_tim* t;
	T_ttab* tt;

	t = (T_tim *)(*handl);
	if (t && D_get_magic(t->flags) == V_tim_mn) {
		D_enter_critical(xmask)
		tt = get_timer_table(t);
		D_assert(tt)
		
		remove_timer_from_table(tt, t);
		t->atick = D_halt_tick;
		D_reset_magic(t->flags);
		
		I_release(t);
		*handl = NULL;
		D_leave_critical(xmask)
	}
}

void I_create_timer(T_tim_handl* handl, 
	INT8U isperiodic, 
	DWORD period, 
	void (*pfunc)(void *uarg), 
	void *uarg) 
{
	DWORD xmask;
	T_tim *t;
	T_ttab* tt;
	
	D_assert(period)
	D_assert(pfunc)
	
	t = (T_tim *)(*handl);
	if (D_get_magic(t->flags) != V_tim_mn) {
		t = create_timer_node(isperiodic, period, pfunc, uarg);
		D_assert(t)
		
		D_enter_critical(xmask)
		t->atick = get_next_tick(period);
		if (t->atick < g_btick)
			tt = g_ttab[1];
		else
			tt = g_ttab[0];
		move_timer_to_table(tt, t);
		*handl = (T_tim_handl)t;
		D_leave_critical(xmask)
	}
}

void timer_handl(void) 
{
	DWORD xmask;
	void (*pfunc)(void* uarg);
	T_tim* t;
	T_ttab* tt;
	
	if (D_test_isof()) {
		D_enter_critical(xmask)
		tt = g_ttab[0];
		g_ttab[0] = g_ttab[1];
		g_ttab[1] = tt;
		
		clean_overflowed_timer_table();
		D_leave_critical(xmask)
	}

	tt = g_ttab[0];
	if (tt->total) {
		t = tt->head->next;
		if (g_btick > t->atick) {
			D_enter_critical(xmask)
			remove_timer_from_table(tt, t);
			
			if (!D_test_timisp(t->flags))
				t->atick = D_halt_tick;
			else {
				t->atick = get_next_tick(t->period);
				if (t->atick < g_btick)
					tt = g_ttab[1];
			}
			move_timer_to_table(tt, t);
			D_leave_critical(xmask)
			
			pfunc = (void (*)(void* ))t->pfunc;
			pfunc(t->uarg);
		}
	}
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
