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

#ifndef __CONFIG_H__
#define __CONFIG_H__


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief All configurations of Regina RTOS defined here for you to tailor.
 *			Make sure that all defines must be non-negative.
 */


#define D_TASK_TIME_SLICE							20u


#define D_CORE_HEART_BEAT							5u
#define D_ENABLE_HEART_BEAT_HOOK					1u


#define D_ENABLE_EXTERNAL_HEAP						0u
#define D_HEAP_TOTAL_SIZE							20 * 1024
#define D_ENABLE_HEAP_CHECK							1u


#define D_STACK_GROW_UP								1u


#define D_IDLE_STACK_SIZE							100u
#define D_ENABLE_IDLE_TASK_HOOK						1u


#define D_TICK_STACK_SIZE							200u
#define D_ENABLE_STACK_CHECK						1u


#define D_ENABLE_SEMAPHORE							1u


#define D_ENABLE_MESSAGE_QUEUE						1u
#define D_MESSAGE_QUEUE_LENGTH						20u


#define D_ENABLE_SOFT_TIMER							1u


#define D_ENABLE_TASK_STAT							1u


#define D_ENABLE_SWITCH_HOOK						1u


#define D_ENABLE_ASSERT_OUTPUT						1u
#ifdef __cplusplus
	}
#endif
#endif
/* ----------------------------------- End of file --------------------------------------- */
