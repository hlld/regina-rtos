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

#ifndef __REGINA_H__
#define __REGINA_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Header file of Regina RTOS configuration */
#include "rr_conf.h"

#ifndef REGINA_STATIC
#define REGINA_STATIC static
#endif
#ifndef REGINA_IMPORT
#define REGINA_IMPORT extern
#endif
#ifndef REGINA_WEAK
#define REGINA_WEAK __weak
#endif
#ifndef REGINA_PACK
#define REGINA_PACK __packed
#endif
#ifndef REGINA_ASSEM
#define REGINA_ASSEM __asm
#endif
#ifndef REGINA_PUBLIC
#define REGINA_PUBLIC
#endif
#ifndef REGINA_PRIVATE
#define REGINA_PRIVATE
#endif

/* Compatible keywords for GCC and CLANG compiler */
#if defined __clang__ || defined __GNUC__
#ifdef __weak
#undef __weak
#endif
#ifndef __weak
#define __weak __attribute__((weak))
#endif
#ifdef __packed
#undef __packed
#endif
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#endif /* __clang__ || __GNUC__ */

/* Basic data type of Regina RTOS */
typedef void* rt_pvoid;
typedef unsigned char rt_uchar;
typedef unsigned short rt_ushort;
typedef unsigned int rt_uint;
typedef volatile unsigned char rt_uvchar;
typedef volatile unsigned short rt_uvshort;
typedef volatile unsigned int rt_uvint;
typedef char rt_char;
typedef short rt_short;
typedef int rt_int;
typedef volatile char rt_vchar;
typedef volatile short rt_vshrot;
typedef volatile int rt_vint;
typedef float rt_float;
typedef double rt_double;

/* Process the D_RTOS_PRESENT_CORE configuration option */
#if D_RTOS_PRESENT_CORE == 1u /* ARM-Cortex-M3 */
#undef D_ARM_CORTEX_CM4F
#undef D_ARM_CORTEX_CM7
#define D_ARM_CORTEX_CM3
#elif D_RTOS_PRESENT_CORE == 2u /* ARM-Cortex-M4F */
#undef D_ARM_CORTEX_CM3
#undef D_ARM_CORTEX_CM7
#define D_ARM_CORTEX_CM4F
#elif D_RTOS_PRESENT_CORE == 3u /* ARM-Cortex-M7 */
#undef D_ARM_CORTEX_CM3
#undef D_ARM_CORTEX_CM4F
#define D_ARM_CORTEX_CM7
#endif /* D_RTOS_PRESENT_CORE */

#if defined D_ARM_CORTEX_CM3 || defined D_ARM_CORTEX_CM4F || defined D_ARM_CORTEX_CM7
/* SP register decreases when pushing stack */
#define D_STACK_GROW_UP 1u

/* Stack label for cheking stack if overflowed */
#define D_TASK_STACK_LABEL 0xA1A2A4A7

#define D_TASK_STACK_OFFSET 8u

/* The maximum of current CPU register */
#define D_MAX_RTOS_TICK 0xFFFFFFFF

/* Data length of current CPU register */
typedef unsigned long rt_size;
#endif /* D_ARM_CORTEX_CM3 || D_ARM_CORTEX_CM4F || D_ARM_CORTEX_CM7 */

/* RTOS use the magic number to classify resources */
#define D_TCB_MAGIC_NUM 0xA1A1
#define D_TIM_MAGIC_NUM 0xA2A2
#define D_SEM_MAGIC_NUM 0xA4A4
#define D_MSG_MAGIC_NUM 0xA7A7

#define rd_test_bit(ctl, mask) ((ctl) & (mask))
#define rd_reset_bit(ctl, mask) (ctl) &= ~(mask)
#define rd_set_bit(ctl, mask) (ctl) |= (mask)
#define rd_reset_bits(ctl, mask) (ctl) &= ~(mask)
#define rd_set_bits(ctl, mask, bits) { (ctl) &= ~(mask); (ctl) |= ((bits) & (mask));}

/**
 * Definitions of the RTOS control flags:
 * Bits 0-15 contains the max task prior
 * Bit 16 -> whether RTOS is running
 * Bit 17 -> whether RTOS needs to schedule task
 * Bit 18 -> whether RTOS tick has overflowd
 * Bit 19 -> whether RTOS core invoked scheduler
 */
#define D_MAX_TASK_PRIOR_MASK 0x0000FFFF
#define D_RTOS_RUNNING_MASK 0x00010000
#define D_TASK_SCHEDULE_MASK 0x00020000
#define D_TICK_OVERFLOW_MASK 0x00040000
#define D_RTOS_INVOKE_MASK 0x00080000

/**
 * Definitions of the task control flags:
 * Bits 0-15 contains task magic number
 * Bit 16 -> whether task needs to be droped
 */
#define D_TASK_MAGIC_MASK 0x0000FFFF
#define D_DROP_TASK_MASK 0x00010000
#define D_WAKE_RESULT_MASK 0x00020000

/* The range of task priority */
#define D_MAX_TASK_PRIOR 0x0000
#define D_MIN_TASK_PRIOR 0xFFFF

/* Options of message or semaphore request */
#define D_BLOCK_TILL_DONE D_MAX_RTOS_TICK
#define D_RET_ONCE_ASK 0u

typedef rt_pvoid rt_task_handle;
typedef void (*rt_task_func)(rt_pvoid parg);
typedef void (*rt_task_exit)(void);

#if D_ENABLE_MESSAGE_QUEUE
/* The struct of data message */
typedef struct message {
	rt_size size;
	rt_pvoid pdata;
} rt_msg; /* struct message */
#endif /* D_ENABLE_MESSAGE_QUEUE */

/* The struct of task control block */
typedef struct task_control_block {
	/* Record the SP register, it must be the first element */
    rt_size* stack_addr;

    rt_uint task_ctl;
    /* The task priority, 0(max)-65535(min) */
    rt_ushort prior;
    /* Store the allocated stack memory address */
    rt_size* base_addr;
    /* Store the table address to which TCB belongs */
    rt_pvoid tcb_table;
    /* Store the tick the task wakes up */
    rt_size wake_tick;

#if D_ENABLE_STACK_CHECK
	/* Store the top stack address of task */
    rt_size* top_addr;
#endif /* D_ENABLE_STACK_CHECK */

#if D_ENABLE_MESSAGE_QUEUE
	rt_msg tcb_msg;
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE
	rt_ushort tcb_prior;
#endif /* D_ENABLE_SEMAPHORE */

#if D_ENABLE_TASK_STAT
	rt_size run_tick;
#endif /* D_ENABLE_TASK_STAT */

    struct task_control_block* next;
    struct task_control_block* last;
} rt_tcb; /* struct task_control_block */

/* The struct of task control block table */
typedef struct tcb_table {
    rt_ushort count;
    rt_tcb* head;
} rt_tcb_table; /* struct tcb_table */

/* The struct of result of RTOS operation */
typedef enum result {
	rt_false = 0,
	rt_true
} rt_result, rt_bool; /* enum result */

#if D_ENABLE_SOFT_TIMER
/**
 * Definitions of the soft timer control flags:
 * Bits 0-15 contains soft timer magic number
 * Bit 16 -> whether timer is periodic
 * Bit 17 -> whether timer is running
 */
#define D_TIMER_MAGIC_MASK 0x0000FFFF
#define D_TIMER_PERIODIC_MASK 0x00010000
#define D_TIMER_RUNNING_MASK 0x00020000

typedef rt_pvoid rt_timer_handle;
typedef void (*rt_timer_func)(rt_pvoid parg);

/* The struct of RTOS soft timer */
typedef struct timer {
	rt_uint	timer_ctl;

	rt_timer_func pfunc;
	rt_pvoid parg;
	rt_size period;
	rt_size wake_tick;
	rt_pvoid tim_table;

	struct timer* next;
	struct timer* last;
} rt_timer; /* struct timer */

/* The struct of RTOS timer table */
typedef struct timer_table {
	rt_ushort count;
	rt_timer* head;
} rt_timer_table; /* struct timer_table */
#endif /* D_ENABLE_SOFT_TIMER */

#if D_ENABLE_MESSAGE_QUEUE
/**
 * Definitions of the message queue control flags:
 * Bits 0-15 contains message queue magic number
 * Bit 16 -> whether queue is empty
 * Bit 17 -> whether queue is full
 */
#define D_MSGQ_MAGIC_MASK 0x0000FFFF
#define D_MSGQ_EMPTY_MASK 0x00010000
#define D_MSGQ_FULL_MASK 0x00020000

typedef rt_pvoid rt_msgq_handle;

/* The struct of message operation type */
typedef enum operation {
	rt_read = 0,
	rt_write
} rt_operate; /* enum operation */

/* The struct of data message queue */
typedef struct queue {
	rt_ushort front;
	rt_ushort rear;
	rt_msg data[D_MESSAGE_QUEUE_LENGTH];
} rt_queue; /* struct queue */

/* The struct of RTOS message queue */
typedef struct message_queue {
	rt_uint msgq_ctl;

	rt_tcb_table* read_list_of;
	rt_tcb_table* read_list;
	rt_tcb_table* write_list_of;
	rt_tcb_table* write_list;
	rt_queue* queue;

	struct message_queue* next;
	struct message_queue* last;
} rt_msgq; /* struct message_queue */

/* The struct of RTOS message queue table */
typedef struct msgq_table {
    rt_ushort count;
    rt_msgq* head;
} rt_msgq_table; /* struct msgq_table */
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE
/**
 * Definitions of the semaphore control flags:
 * Bits 0-15 contains semaphore magic number
 * Bit 16 -> whether semaphore voids mutex
 * Bit 17 -> whether semaphore has been held
 */
#define D_MUTEX_MAGIC_MASK 0x0000FFFF
#define D_MUTEX_VOID_MASK 0x00010000
#define D_MUTEX_HOLD_MASK 0x00020000

typedef rt_pvoid rt_mutex_handle;
typedef rt_pvoid rt_lock_handle;

/* The struct of RTOS mutex */
typedef struct mutex {
	rt_uint mutex_ctl;

	rt_tcb_table* request_list_of;
	rt_tcb_table* request_list;
	rt_tcb*	holder;

	struct mutex* next;
	struct mutex* last;
} rt_mutex; /* struct mutex */

/* The struct of RTOS mutex table */
typedef struct mutex_table {
    rt_ushort count;
    rt_mutex* head;
} rt_mutex_table; /* struct mutex_table */
#endif /* D_ENABLE_SEMAPHORE */

REGINA_IMPORT rt_size rr_task_lock;
REGINA_IMPORT void rf_print(const rt_char* ptext, ...);
REGINA_IMPORT rt_size rf_enter_critical(void);
REGINA_IMPORT void rf_leave_critical(rt_size mask);

#define rd_task_lock() { rr_task_lock++;}
#define rd_task_unlock() { rd_assert(rr_task_lock) rr_task_lock--;}

#define rd_enter_critical(x) { (x) = rf_enter_critical();}
#define rd_leave_critical(x) { rf_leave_critical((x));}

#if D_ENABLE_ASSERT
#define rd_assert(x) { \
    if (!(x)) { \
        rf_print("Regina Error: %s, %d\n", __FILE__, __LINE__); \
    } \
}
#else
#define rd_assert(x)
#endif /* D_ENABLE_ASSERT */

/* Declared Regina public functions */
REGINA_PUBLIC void rf_handle_tick_isr(void);
REGINA_PUBLIC rt_pvoid rf_alloc(rt_size xBytes);
REGINA_PUBLIC void rf_free(rt_pvoid pL);
REGINA_PUBLIC rt_size rf_get_taskid(void);
REGINA_PUBLIC rt_size rf_get_taskid_isr(rt_task_handle handl);
REGINA_PUBLIC rt_size rf_get_free_heap_size(void);
REGINA_PUBLIC rt_size rf_get_lowest_free_heap_size(void);
REGINA_PUBLIC rt_size rf_get_rtos_tick(void);
REGINA_PUBLIC void rf_block_usec(rt_uint xusec);
REGINA_PUBLIC void rf_block_msec(rt_uint xmsec);

REGINA_PUBLIC void rf_setup_rtos(void);
REGINA_PUBLIC void rf_start_rtos(void);
REGINA_PUBLIC void rf_create_task_inner (
	rt_task_handle* handle, 
	rt_ushort prior, 
	rt_ushort stk_size, 
	rt_task_func pfunc, 
	rt_pvoid parg
);
/* REGINA_PUBLIC */ #define rf_create_task(handle, prior, stk_size, pfunc, parg) \
	rf_create_task_inner(&handle, prior, stk_size, pfunc, parg)
REGINA_PUBLIC void rf_sleep(rt_size xmsec);
REGINA_PUBLIC void rf_drop_task_inner(rt_task_handle* handle);
/* REGINA_PUBLIC */ #define rf_drop_task(handle) rf_drop_task_inner(&handle)
REGINA_PUBLIC void rf_exit(void);
REGINA_PUBLIC void rf_restore_task(rt_task_handle handle);
REGINA_PUBLIC void rf_suspend_task(rt_task_handle handle);
REGINA_PUBLIC void rf_schedule(void);

#if D_ENABLE_TASK_STAT
REGINA_PUBLIC rt_size rf_get_task_run_tick(rt_task_handle handle);
REGINA_PUBLIC rt_float rf_get_task_run_rate(rt_task_handle handle);
REGINA_PUBLIC rt_size rf_get_core_run_tick(void);
REGINA_PUBLIC rt_float rf_get_core_run_rate(void);
REGINA_PUBLIC rt_size rf_get_idle_run_tick(void);
REGINA_PUBLIC rt_float rf_get_idle_run_rate(void);
REGINA_PUBLIC rt_size rf_get_run_tick(void);
REGINA_PUBLIC rt_float rf_get_run_rate(void);
#endif /* D_ENABLE_TASK_STAT */

#if D_ENABLE_SOFT_TIMER
REGINA_PUBLIC void rf_set_timer_period(rt_timer_handle handle, rt_size period);
REGINA_PUBLIC void rf_set_timer_arg(rt_timer_handle handle, rt_pvoid parg);
REGINA_PUBLIC void rf_create_timer_inner (
	rt_timer_handle* handle, 
	rt_size period, 
	rt_timer_func pfunc, 
	rt_pvoid parg
);
/* REGINA_PUBLIC */ #define rf_create_timer(handle, period, pfunc, parg) \
	rf_create_timer_inner(&handle, period, pfunc, parg)
REGINA_PUBLIC void rf_create_single_timer_inner (
	rt_timer_handle* handle, 
	rt_size period, 
	rt_timer_func pfunc, 
	rt_pvoid parg
);
/* REGINA_PUBLIC */ #define rf_create_single_timer(handle, period, pfunc, parg) \
	rf_create_single_timer_inner(&handle, period, pfunc, parg)
REGINA_PUBLIC void rf_start_timer(rt_timer_handle handle);
REGINA_PUBLIC void rf_stop_timer(rt_timer_handle handle);
REGINA_PUBLIC void rf_drop_timer_inner(rt_timer_handle* handle);
/* REGINA_PUBLIC */ #define rf_drop_timer(handle) rf_drop_timer_inner(&handle)
#endif /* D_ENABLE_SOFT_TIMER */

#if D_ENABLE_MESSAGE_QUEUE
REGINA_PUBLIC void rf_create_msgq_inner(rt_msgq_handle* handle);
/* REGINA_PUBLIC */ #define rf_create_msgq(handle) rf_create_msgq_inner(&handle)
REGINA_PUBLIC rt_result rf_send_message(rt_msgq_handle handle, rt_msg* msg, rt_size xmsec);
REGINA_PUBLIC rt_result rf_send_message_isr(rt_msgq_handle handle, rt_msg* msg);
REGINA_PUBLIC rt_result rf_receive_message(rt_msgq_handle handle, rt_msg* msg, rt_size xmsec);
REGINA_PUBLIC void rf_drop_msgq_inner(rt_msgq_handle* handle);
/* REGINA_PUBLIC */ #define rf_drop_msgq(handle) rf_drop_msgq_inner(&handle)
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE
REGINA_PUBLIC void rf_create_mutex_inner(rt_mutex_handle* handle);
/* REGINA_PUBLIC */ #define rf_create_mutex(handle) rf_create_mutex_inner(&handle)
REGINA_PUBLIC rt_result rf_release_mutex(rt_mutex_handle handle);
REGINA_PUBLIC rt_result rf_request_mutex(rt_mutex_handle handle, rt_size xmsec);
REGINA_PUBLIC void rf_drop_mutex_inner(rt_mutex_handle* handle);
/* REGINA_PUBLIC */ #define rf_drop_mutex(handle) rf_drop_mutex_inner(&handle)

REGINA_PUBLIC void rf_create_lock_inner(rt_mutex_handle* handle);
/* REGINA_PUBLIC */ #define rf_create_lock(handle) rf_create_lock_inner(&handle)
/* REGINA_PUBLIC */ #define rf_release_lock(handle) rf_release_mutex(handle)
REGINA_PUBLIC rt_result rf_release_lock_isr(rt_lock_handle handle);
/* REGINA_PUBLIC */ #define rf_request_lock(handle, xmsec) rf_request_mutex(handle, xmsec)
/* REGINA_PUBLIC */ #define rf_drop_lock(handle) rf_drop_mutex_inner(&handle)
#endif /* D_ENABLE_SEMAPHORE */

#ifdef __cplusplus
	}
#endif

#endif /* __REGINA_H__ */

/********************************** End of The File ************************************/
