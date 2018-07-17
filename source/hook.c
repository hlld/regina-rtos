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


#if D_ENABLE_SWITCH_HOOK
__weak void task_switch_in_hook(T_task_handl handl)
{
}

__weak void task_switch_out_hook(T_task_handl handl)
{
}
#endif

#if D_ENABLE_IDLE_TASK_HOOK
__weak void IDLE_task_hook(void)
{
}
#endif

#if D_ENABLE_HEART_BEAT_HOOK
__weak void tick_irq_hook(void)
{
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
