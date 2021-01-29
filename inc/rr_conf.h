/**
 * **************************************************************************************
 * Regina V3.0 - Copyright (C) 2019 Hlld.
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
 * **************************************************************************************
 */

#ifndef __RR_CONF_H__
#define __RR_CONF_H__


/**
 * Definition of the CPU core based on the current situation
 * At present, Regina supports three CPU cores as follows:
 *     ARM-Cortex-M3=1u
 *     ARM-Cortex-M4F=2u
 *     ARM-Cortex-M7=3u
 */
#define D_RTOS_PRESENT_CORE 3u /* ARM-Cortex-M7 */


/**
 * Definition of the heartbeat of RTOS
 * RTOS will process core tasks every D_CORE_HEART_BEAT millisecond
 */
#define D_CORE_HEART_BEAT 10u


/**
 * Definition of the time slice of RTOS 
 * Tasks with the same priority will run in a time slice rotation
 * The unit defined default to millisecond
 */
#define D_TASK_TIME_SLICE 40u


/**
 * Definition of the total size of RTOS heap
 * RTOS will alloc memory from heap when invoke function rf_alloc()
 * The unit defined default to bytes
 */
#define D_TOTAL_HEAP_SIZE (20*1024)


/**
 * Definition of the use of external RTOS heap
 * Set D_ENABLE_EXTERNAL_HEAP to 1u to use external RTOS heap 
 */
#define D_ENABLE_EXTERNAL_HEAP 0u


/**
 * Definition of checking whether RTOS heap is empty 
 * Set D_ENABLE_HEAP_CHECK to 0u to disable RTOS heap check
 */
#define D_ENABLE_HEAP_CHECK 1u


/**
 * Definition of the stack size of core tasks
 * The unit defined default to 4*bytes
 */
#define D_IDLE_TASK_STACK_SIZE 100u
#define D_TICK_TASK_STACK_SIZE 200u


/**
 * Definition of hook functions of RTOS
 * Set D_ENABLE_IDLE_TASK_HOOK to 0u to disable the IDLE task hook
 * Set D_ENABLE_HEART_BEAT_HOOK to 0u to disable the heartbeat hook
 */
#define D_ENABLE_IDLE_TASK_HOOK 1u
#define D_ENABLE_HEART_BEAT_HOOK 1u


/**
 * Definition of checking whether task stack has overflowed
 * Set D_ENABLE_STACK_CHECK to 0u to disable task stack check
 */
#define D_ENABLE_STACK_CHECK 1u


/**
 * Definition of whether to use the RTOS software timer
 * Set D_ENABLE_SOFT_TIMER to 0u to disable software timer
 */
#define D_ENABLE_SOFT_TIMER 1u


/**
 * Definition of whether to use the RTOS message queue
 * Set D_ENABLE_MESSAGE_QUEUE to 0u to disable message queue
 */
#define D_ENABLE_MESSAGE_QUEUE 1u
#define D_MESSAGE_QUEUE_LENGTH 20u


/**
 * Definition of whether to use the RTOS semaphore
 * Set D_ENABLE_MESSAGE_QUEUE to 0u to disable semaphore
 */
#define D_ENABLE_SEMAPHORE 1u


/**
 * Definition of whether to enable the RTOS task statistics
 * Set D_ENABLE_TASK_STAT to 0u to disable statistics
 */
#define D_ENABLE_TASK_STAT 1u


/**
 * Definition of the function of RTOS assert
 * Set D_ENABLE_ASSERT to 0u will disable function rd_assert()
 */
#define D_ENABLE_ASSERT 1u


#endif /* __RR_CONF_H__ */

/********************************** End of The File ************************************/
