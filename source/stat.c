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


#if D_ENABLE_TASK_STAT
DWORD I_get_IDLE_rtick(void) 
{
	return g_IDLE->rtick;
}

FLOAT I_get_IDLE_rrate(void)
{
	return g_IDLE->rate;
}

DWORD I_get_rttask_rtick(void)
{
	return g_rttask->rtick;
}

FLOAT I_get_rttask_rrate(void)
{
	return g_rttask->rate;
}

void task_stat(T_tcb* tcb) 
{
	DWORD tick;

	tick = g_ltmark;
	if (g_btick < tick)
		tcb->rtick += D_halt_tick - tick + g_btick;
	else
		tcb->rtick += g_btick - tick;
	
	if (g_btick) {
		tcb->rate = (FLOAT)(tcb->rtick) / (FLOAT)g_btick;
		if (tcb->rate > 1) {
			tcb->rate = 0;
			tcb->rtick = 0;
		}
	}
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
