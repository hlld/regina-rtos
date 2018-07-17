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


void I_sleep_ms(FLOAT xms)
{
	sleep_us((INT32U)(xms * 1000));
}

void tick_handl(void)
{
	DWORD xmask;
	
	D_enter_critical(xmask)
	if (D_test_run()) {
#if D_ENABLE_HEART_BEAT_HOOK
		tick_irq_hook();
#endif
		g_btick += D_CORE_HEART_BEAT;
		if (!g_slock && g_rttask != g_TICK) {
#if D_ENABLE_STACK_CHECK
			stack_overflow_check(g_rttask);
#endif
#if D_ENABLE_TASK_STAT
			task_stat(g_rttask);
#endif
#if D_ENABLE_SWITCH_HOOK
			task_switch_out_hook(g_rttask);
#endif
			g_temp = g_rttask;
			g_rdtask = g_TICK;
#if D_ENABLE_SWITCH_HOOK
			task_switch_in_hook(g_rdtask);
#endif
#if D_ENABLE_TASK_STAT
			g_ltmark = g_btick;
#endif
			task_switch();
		}
	}
	D_leave_critical(xmask)
}

DWORD get_next_tick(DWORD tick) 
{
	DWORD next, time;

	if (tick > D_CORE_HEART_BEAT)
		tick -= D_CORE_HEART_BEAT;
	else
		tick = 0;
	
	time = g_btick;
	next = time + tick;
	
	if (next < g_btick)
		next = tick - (D_halt_tick - time);
	
	if (next == D_halt_tick)
		next = 0;
	return next;
}
/* ----------------------------------- End of file --------------------------------------- */
