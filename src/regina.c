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

#include "../inc/regina.h"

REGINA_IMPORT void rf_puts(const rt_char* pstr, rt_size xlen);
REGINA_IMPORT void rf_delay_us(rt_uint xusec);
REGINA_IMPORT void rf_delay_ms(rt_uint xmsec);
REGINA_IMPORT rt_size* rf_setup_task_stack (
	rt_task_func pfunc, 
	rt_pvoid parg, 
	rt_size* stk_addr, 
	rt_task_exit pexit
);
REGINA_IMPORT void rf_setup_tick(void);
REGINA_IMPORT void rf_start_core(void);
REGINA_IMPORT void rf_switch_context(void);

REGINA_PRIVATE rt_size rr_tick;
REGINA_PRIVATE rt_size rr_last_tick;
REGINA_PRIVATE rt_size rr_task_lock;
REGINA_PRIVATE rt_size rr_task_tick;
REGINA_PRIVATE rt_uint rr_ctl;

#if D_ENABLE_TASK_STAT
REGINA_PRIVATE rt_size rr_load_tick;
#endif /* D_ENABLE_TASK_STAT */

REGINA_STATIC rt_tcb_table* rr_rt_ttable;
REGINA_STATIC rt_tcb_table* rr_sleep_ttable_of;
REGINA_STATIC rt_tcb_table* rr_sleep_ttable;

#if D_ENABLE_SOFT_TIMER
REGINA_STATIC rt_timer_table* rr_timer_table_of;
REGINA_STATIC rt_timer_table* rr_timer_table;
#endif /* D_ENABLE_SOFT_TIMER */

#if D_ENABLE_MESSAGE_QUEUE
REGINA_STATIC rt_msgq_table* rr_msgq_table;
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE
REGINA_STATIC rt_mutex_table* rr_mutex_table;
#endif /* D_ENABLE_SEMAPHORE */

REGINA_PRIVATE rt_tcb* rr_idle_task;
REGINA_PRIVATE rt_tcb* rr_tick_task;
REGINA_PRIVATE rt_tcb* rr_pt_task;
REGINA_PRIVATE rt_tcb* rr_pc_task;
REGINA_PRIVATE rt_tcb* rr_rd_task;
REGINA_PRIVATE rt_tcb* rr_rt_task;

#define D_HEAP_ALLOC_LABEL 0x80000000
#define D_HEAP_BYTE_ALIGN 8u
#define D_HEAP_BYTE_ALIGN_MASK 0x07

/* Align target to D_HEAP_BYTE_ALIGN bytes */
#define rd_byte_align(x, byte, mask) ((x) + (byte) - 1) & ~(mask)
#define rd_align(x) rd_byte_align(x, D_HEAP_BYTE_ALIGN, D_HEAP_BYTE_ALIGN_MASK)

/* List struct of RTOS heap memory */
typedef struct heap_list {
	rt_uint size;
	struct heap_list* next;
} rt_heap_list; /* struct heap_list */

#if D_ENABLE_EXTERNAL_HEAP
REGINA_IMPORT rt_uchar rr_heap[D_TOTAL_HEAP_SIZE];
#else
REGINA_STATIC rt_uchar rr_heap[D_TOTAL_HEAP_SIZE];
#endif /* D_ENABLE_EXTERNAL_HEAP */

REGINA_STATIC rt_size rr_heap_list_struct_size;
REGINA_STATIC rt_size rr_free_heap_size;
REGINA_STATIC rt_size rr_lowest_free_heap_size;
REGINA_STATIC rt_heap_list* rr_phead;
REGINA_STATIC rt_heap_list* rr_ptail;

/* Definitions of operations of RTOS core flags */
#define rd_get_max_prior() ((rr_ctl & D_MAX_TASK_PRIOR_MASK) >> 0)
#define rd_reset_max_prior() rd_reset_bits(rr_ctl, D_MAX_TASK_PRIOR_MASK)
#define rd_set_max_prior(prior) rd_set_bits(rr_ctl, D_MAX_TASK_PRIOR_MASK, prior)

#define rd_test_rtos_running() rd_test_bit(rr_ctl, D_RTOS_RUNNING_MASK)
#define rd_reset_rtos_running() rd_reset_bit(rr_ctl, D_RTOS_RUNNING_MASK)
#define rd_set_rtos_running() rd_set_bit(rr_ctl, D_RTOS_RUNNING_MASK)

#define rd_test_task_schedule() rd_test_bit(rr_ctl, D_TASK_SCHEDULE_MASK)
#define rd_reset_task_schedule() rd_reset_bit(rr_ctl, D_TASK_SCHEDULE_MASK)
#define rd_set_task_schedule() rd_set_bit(rr_ctl, D_TASK_SCHEDULE_MASK)

#define rd_test_tick_overflow() rd_test_bit(rr_ctl, D_TICK_OVERFLOW_MASK)
#define rd_reset_tick_overflow() rd_reset_bit(rr_ctl, D_TICK_OVERFLOW_MASK)
#define rd_set_tick_overflow() rd_set_bit(rr_ctl, D_TICK_OVERFLOW_MASK)

#define rd_test_rtos_invoke() rd_test_bit(rr_ctl, D_RTOS_INVOKE_MASK)
#define rd_reset_rtos_invoke() rd_reset_bit(rr_ctl, D_RTOS_INVOKE_MASK)
#define rd_set_rtos_invoke() rd_set_bit(rr_ctl, D_RTOS_INVOKE_MASK)

/* Definitions of operations of RTOS task flags */
#define rd_get_task_magic(task) (((task)->task_ctl & D_TASK_MAGIC_MASK) >> 0)
#define rd_reset_task_magic(task) rd_reset_bits((task)->task_ctl, D_TASK_MAGIC_MASK)
#define rd_set_task_magic(task, magic) rd_set_bits((task)->task_ctl, D_TASK_MAGIC_MASK, magic)

#define rd_test_drop_task(task) rd_test_bit((task)->task_ctl, D_DROP_TASK_MASK)
#define rd_reset_drop_task(task) rd_reset_bit((task)->task_ctl, D_DROP_TASK_MASK)
#define rd_set_drop_task(task) rd_set_bit((task)->task_ctl, D_DROP_TASK_MASK)

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
#define rd_test_wake_result(task) rd_test_bit((task)->task_ctl, D_WAKE_RESULT_MASK)
#define rd_reset_wake_result(task) rd_reset_bit((task)->task_ctl, D_WAKE_RESULT_MASK)
#define rd_set_wake_result(task) rd_set_bit((task)->task_ctl, D_WAKE_RESULT_MASK)
#endif /* D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE */

REGINA_PRIVATE void rf_print(const rt_char* ptext, ...)
{
    rt_char pbuf[128];
	va_list list;

    rd_assert(ptext)

	/* Length of ptext must be smaller than 128 */
	memset(pbuf, 0, 128);

	va_start(list, ptext);
	vsprintf(pbuf, ptext, list);
	va_end(list);

	rf_puts(pbuf, strlen(pbuf));
}

#if D_ENABLE_STACK_CHECK
REGINA_STATIC void rf_check_stack_overflow(rt_tcb* ptcb)
{
	rd_assert(ptcb)

#if D_STACK_GROW_UP
	if (*(ptcb->top_addr + D_TASK_STACK_OFFSET) != D_TASK_STACK_LABEL) {
#else
	if (*(ptcb->top_addr - D_TASK_STACK_OFFSET) != D_TASK_STACK_LABEL) {
#endif /* D_STACK_GROW_UP */
		rf_print("Regina Warning: %X->task stack overflowed\n", (rt_size)ptcb);
_DEAD_LOOP:
    	goto _DEAD_LOOP;
	}
}
#endif /* D_ENABLE_STACK_CHECK */

#if D_ENABLE_HEAP_CHECK
REGINA_STATIC void rf_heap_check_empty(void)
{
    if (rr_free_heap_size < (rr_heap_list_struct_size << 2)) {
		rf_print("Regina Warning: the RTOS heap is empty\n");
_DEAD_LOOP:
		goto _DEAD_LOOP;
	}
}
#endif /* D_ENABLE_HEAP_CHECK */

#if D_ENABLE_IDLE_TASK_HOOK
REGINA_WEAK void rf_free_stack_hook(void) {
}
#endif /* D_ENABLE_IDLE_TASK_HOOK */

#if D_ENABLE_HEART_BEAT_HOOK
REGINA_WEAK void rf_heart_beat_hook(void) {
}
#endif /* D_ENABLE_HEART_BEAT_HOOK */

#if D_ENABLE_TASK_STAT
REGINA_PUBLIC rt_size rf_get_task_run_tick(rt_task_handle handle)
{
	rt_size result = 0;
	rt_tcb* ptcb;

	ptcb = (rt_tcb *)handle;
	if (ptcb && rd_get_task_magic(ptcb) == D_TCB_MAGIC_NUM) {
		result = ptcb->run_tick;
	}
	return result;
}

REGINA_PUBLIC rt_float rf_get_task_run_rate(rt_task_handle handle)
{
	rt_float result = 0;
	rt_tcb* ptcb;

	ptcb = (rt_tcb *)handle;
	if (ptcb && rd_get_task_magic(ptcb) == D_TCB_MAGIC_NUM && rr_tick) {
		result = (rt_float)(ptcb->run_tick) / (rt_float)rr_tick * 100;
	}
	return result;
}

REGINA_PUBLIC rt_size rf_get_core_run_tick(void) {
	return rr_tick_task->run_tick;
}

REGINA_PUBLIC rt_float rf_get_core_run_rate(void)
{
	rt_float result = 0;

	if (rr_tick) {
		result = (rt_float)(rr_tick_task->run_tick) / (rt_float)rr_tick * 100;
	}
	return result;
}

REGINA_PUBLIC rt_size rf_get_idle_run_tick(void) {
	return rr_idle_task->run_tick;
}

REGINA_PUBLIC rt_float rf_get_idle_run_rate(void)
{
	rt_float result = 0;

	if (rr_tick) {
		result = (rt_float)(rr_idle_task->run_tick) / (rt_float)rr_tick * 100;
	}
	return result;
}

REGINA_PUBLIC rt_size rf_get_run_tick(void) {
	return rr_rt_task->run_tick;
}

REGINA_PUBLIC rt_float rf_get_run_rate(void)
{
	rt_float result = 0;

	if (rr_tick) {
		result = (rt_float)(rr_rt_task->run_tick) / (rt_float)rr_tick * 100;
	}
	return result;
}

REGINA_STATIC void rf_task_tick_stat(rt_tcb* ptcb)
{
	rt_bool is_reset = rt_false;
	rt_size load, tick;

	rd_assert(ptcb)

	load = rr_load_tick;
	tick = rr_tick;

	if (tick < load) {
		ptcb->run_tick += D_MAX_RTOS_TICK - load + tick;
		is_reset = rt_true;
	}
	else {
		ptcb->run_tick += tick - load;
	}

	/* Reset task statistics if tick has overflowed */
	if (is_reset == rt_true) {
		ptcb->run_tick = 0;
	}
}
#endif /* D_ENABLE_TASK_STAT */

REGINA_PUBLIC rt_size rf_get_lowest_free_heap_size(void) {
	return rr_lowest_free_heap_size;
}

REGINA_PUBLIC rt_size rf_get_free_heap_size(void) {
	return rr_free_heap_size;
}

REGINA_STATIC void rf_link_free_heap_list(rt_heap_list* pL)
{
    rt_heap_list* list;
    rt_uchar* pt;

    rd_assert(pL)

	for(list = rr_phead; list->next < pL; list = list->next) {
		/* Do nothing but find a appropriate position */
	}

	pt = (rt_uchar *)list;
	if((pt + list->size) == (rt_uchar *)pL) {
		list->size += pL->size;
		pL = list;
	}

	pt = (rt_uchar *) pL;
	if((pt + pL->size) == (rt_uchar *)(list->next)) {
		if(list->next != rr_ptail) {
			pL->size += list->next->size;
			pL->next = list->next->next;
		}
		else {
            pL->next = rr_ptail;
        }
	}
	else {
        pL->next = list->next;
    }

	if(list != pL) {
        list->next = pL;
    }
}

REGINA_PRIVATE void rf_setup_heap(void)
{
    rt_size addr, total;
    rt_uchar* pt;

    rr_heap_list_struct_size = rd_align(sizeof(rt_heap_list));
	addr = (rt_size)rr_heap;
	total = D_TOTAL_HEAP_SIZE;
	if (addr & D_HEAP_BYTE_ALIGN_MASK) {
		addr = rd_align(addr);
		total -= addr - (rt_size)rr_heap;
	}
	pt = (rt_uchar *)addr;
	rr_phead = (rt_heap_list *)addr;
	addr += rr_heap_list_struct_size;
	rr_phead->size = 0;
	rr_phead->next = (rt_heap_list *)addr;

	addr += (total - (rr_heap_list_struct_size << 1));
	addr &= ~D_HEAP_BYTE_ALIGN_MASK;
	rr_ptail = (rt_heap_list*)addr;
	rr_ptail->size = 0;
	rr_ptail->next = NULL;

	rr_phead->next->next = rr_ptail;
	rr_phead->next->size = (addr - (rt_size)pt - rr_heap_list_struct_size);
	rr_free_heap_size = rr_phead->next->size;
	rr_lowest_free_heap_size = rr_free_heap_size;
}

REGINA_PUBLIC void rf_free(rt_pvoid pL)
{
	rt_heap_list* list;
	rt_uchar* pct;
	rt_size csr;

	rd_assert(pL)
	rd_enter_critical(csr)

    /* Find the heap list struct from pL */
	pct = (rt_uchar *)pL;
	pct -= rr_heap_list_struct_size;
	list = (rt_heap_list *)pct;

	rd_assert(list->size & D_HEAP_ALLOC_LABEL)
	rd_assert(!list->next)

    /* Make sure pL is from RTOS heap */
	if (!list->next && (list->size & D_HEAP_ALLOC_LABEL)) {
		list->size &= ~D_HEAP_ALLOC_LABEL;

		rr_free_heap_size += list->size;
		rf_link_free_heap_list(list);
	}
	rd_leave_critical(csr)
}

REGINA_PUBLIC rt_pvoid rf_alloc(rt_size xBytes)
{
	rt_pvoid result = NULL;
	rt_heap_list* plast, *pL, *pnew;
	rt_size csr;

	rd_assert(xBytes)
	rd_enter_critical(csr)

	xBytes += rr_heap_list_struct_size;
	if (xBytes & D_HEAP_BYTE_ALIGN_MASK) {
		xBytes += (D_HEAP_BYTE_ALIGN - (xBytes & D_HEAP_BYTE_ALIGN_MASK));
        /* Assert xBytes has been aligned */
		rd_assert(!(xBytes & D_HEAP_BYTE_ALIGN_MASK))
	}

	if (xBytes <= rr_free_heap_size) {
		plast = rr_phead;
		pL = rr_phead->next;

		while((pL->size < xBytes) && pL->next) {
			plast = pL;
			pL = pL->next;
		}

		if(pL != rr_ptail) {
			result = (rt_pvoid)((rt_size)(plast->next) + rr_heap_list_struct_size);
			rd_assert(!((rt_size)result & D_HEAP_BYTE_ALIGN_MASK))

			plast->next = pL->next;
			if((pL->size - xBytes) > (rr_heap_list_struct_size << 1)) {
				pnew = (rt_heap_list *)((rt_size)pL + xBytes);
				rd_assert(!((rt_size)pnew & D_HEAP_BYTE_ALIGN_MASK))

				pnew->size = pL->size - xBytes;
				pL->size = xBytes;
				rf_link_free_heap_list(pnew);
			}

			rr_free_heap_size -= pL->size;
			if(rr_free_heap_size < rr_lowest_free_heap_size) {
				rr_lowest_free_heap_size = rr_free_heap_size;
			}
			pL->size |= D_HEAP_ALLOC_LABEL;
			pL->next = NULL;
		}
	}
	rd_leave_critical(csr)
#if D_ENABLE_HEAP_CHECK
	rf_heap_check_empty();
#endif /* D_ENABLE_HEAP_CHECK */
	return result;
}

REGINA_PUBLIC rt_size rf_get_taskid(void) {
	return (rt_size)rr_rt_task;
}

REGINA_PUBLIC rt_size rf_get_taskid_isr(rt_task_handle handle)
{
	rt_size result = 0;
	rt_tcb* ptcb;

	ptcb = (rt_tcb *)handle;
	if (ptcb && rd_get_task_magic(ptcb) == D_TCB_MAGIC_NUM) {
		result = (rt_size)ptcb;
	}
	return result;
}

REGINA_PUBLIC rt_size rf_get_rtos_tick(void) {
	return rr_tick;
}

REGINA_PUBLIC void rf_block_usec(rt_uint xusec) 
{
	rd_task_lock()
	rf_delay_us(xusec);
	rd_task_unlock()
}

REGINA_PUBLIC void rf_block_msec(rt_uint xmsec) 
{
	rd_task_lock()
	rf_delay_ms(xmsec);
	rd_task_unlock()
}

REGINA_STATIC rt_tcb* rf_create_tcb (
	rt_size* stack_addr, 
	rt_ushort prior, 
	rt_size* base_addr
) {
    rt_tcb* ptcb;

    rd_assert(stack_addr)
    rd_assert(prior <= D_MIN_TASK_PRIOR)
    rd_assert(base_addr)

    ptcb = (rt_tcb *)rf_alloc(sizeof(rt_tcb));
    rd_assert(ptcb)

    rd_set_task_magic(ptcb, D_TCB_MAGIC_NUM);
	rd_reset_drop_task(ptcb);

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
	rd_reset_wake_result(ptcb);
#endif /* D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE */

	ptcb->stack_addr = stack_addr;
    ptcb->prior = prior;
    ptcb->base_addr = base_addr;
    ptcb->tcb_table = NULL;
    ptcb->wake_tick = 0;

#if D_ENABLE_STACK_CHECK
    ptcb->top_addr = NULL;
#endif /* D_ENABLE_STACK_CHECK */

#if D_ENABLE_SEMAPHORE
	ptcb->tcb_prior = prior;
#endif /* D_ENABLE_SEMAPHORE */

#if D_ENABLE_TASK_STAT
	ptcb->run_tick = 0;
#endif /* D_ENABLE_TASK_STAT */

    ptcb->last = NULL;
    ptcb->next = NULL;

    return ptcb;
}

REGINA_STATIC rt_tcb_table* rf_create_tcb_table(void)
{
    rt_tcb_table* ptable;

    ptable = (rt_tcb_table *)rf_alloc(sizeof(rt_tcb_table));
    rd_assert(ptable)

    ptable->count = 0;
    ptable->head = NULL;

    return ptable;
}

REGINA_STATIC void rf_insert_tcb_into_table(rt_tcb_table* ptable, rt_tcb* ptcb)
{
    rt_bool is_prior = rt_false;
    rt_tcb* pt, *list;

    rd_assert(ptable)
    rd_assert(ptcb)

    /**
     * Only Tasks in rr_rt_ttable are sorted by prior
     * Tasks in other tables are sorted by wake_tick
     * Task sort always in an ascending order
     */
    if (ptable == rr_rt_ttable) {
        is_prior = rt_true;
    }

    if (!ptable->count) {
        rd_assert(!ptable->head)
        /**
         * If table is empty then set ptcb as table head, and
         * link ptcb to a circle
         */
        ptable->head = ptcb;
        ptcb->next = ptcb;
        ptcb->last = ptcb;

        if (ptable == rr_rt_ttable) {
            rr_pc_task = ptcb;
            /**
             * When rr_rt_ttable has a task with higher priority, then
             * set both RTOS max priority and task schedule flag
             */
            rd_set_max_prior(ptcb->prior);
            rd_set_task_schedule();
        }
    }
    else { /* if (ptable->count) */
        rd_assert(ptable->head)

        list = ptable->head;
		pt = NULL;
		do {
            if (is_prior == rt_true) {
                if (ptcb->prior < list->prior) {
				    pt = list;
				    break;
			    }
            }
            else if (ptcb->wake_tick < list->wake_tick) {
				pt = list;
				break;
			}
			list = list->next;
		} while (list != ptable->head);
        /**
         * Not find the suitable position to insert, then
         * append to the tail of TCB table
         */
		if (pt == NULL) {
			pt = ptable->head;
		}
        /**
         * The insert point is the head of TCB table, then
         * make sure that the head contains the min sort number
         */
		else if (pt == ptable->head) {
			ptable->head = ptcb;
		}

		ptcb->next = pt;
		ptcb->last = pt->last;
		pt->last = ptcb;
        ptcb->last->next = ptcb;

        /* The global max priority must initiate to D_MIN_TASK_PRIOR */
        if (ptable == rr_rt_ttable && ptcb->prior < rd_get_max_prior()) {
            rd_set_max_prior(ptcb->prior);
            rd_set_task_schedule();
        }
    }
    /* Update the count of task table */
    ptable->count++;
	ptcb->tcb_table = ptable;
}

REGINA_STATIC void rf_remove_tcb_from_table(rt_tcb_table* ptable, rt_tcb* ptcb)
{
    rd_assert(ptable)
    rd_assert(ptcb)

	rd_assert((rt_pvoid)ptable == ptcb->tcb_table)
	rd_assert(ptable->count)
    if (ptable->count == 1) {
        rd_assert(ptcb == ptable->head)
        ptable->head = NULL;

        if (ptable == rr_rt_ttable) {
            rr_pc_task = NULL;
            /**
             * Set the max prior to D_MIN_TASK_PRIOR when rr_rt_ttable is empty
             * The priority of IDLE task is D_MIN_TASK_PRIOR
             */
            rd_set_max_prior(D_MIN_TASK_PRIOR);
        }
    }
    else if (ptable->count > 1) {
        if (ptable == rr_rt_ttable) {
            rd_assert(ptable->head->prior == rd_get_max_prior())
            /**
             * Tasks in rr_rt_ttable have sorted in an ascending order
             * Regina alows tasks with the same priority to run time slice alternately
             * RTOS needs to check if table has tasks with the same priority
             */
            if (ptcb == ptable->head && ptcb->next->prior != rd_get_max_prior()) {
                rd_set_max_prior(ptcb->next->prior);
            }

            if (ptcb == rr_pc_task) {
                rr_pc_task = ptcb->next;
            }
        }
        /* Assert Tasks in tables have sorted in an ascending order */
        if (ptcb == ptable->head) {
            ptable->head = ptcb->next;
        }
    }
	ptcb->last->next = ptcb->next;
	ptcb->next->last = ptcb->last;
	ptcb->last = NULL;
	ptcb->next = NULL;

	rd_assert(ptable->count)
	ptable->count--;
    ptcb->tcb_table = NULL;
}

REGINA_STATIC rt_tcb* rf_get_ready_task(void)
{
    rt_tcb_table *ptable;
    rt_tcb *ptcb;

	ptable = rr_rt_ttable;
	if(ptable->count) {
        rd_assert(rr_pc_task)
        /**
         * Always select the task with higher priority than rr_pc_task
         * Task table has sorted by task priority (from high to low)
         */
		if (rr_pc_task->prior > rd_get_max_prior()) {
            rr_pc_task = ptable->head;
        }
		ptcb = rr_pc_task;
        /* Heads to the next Task in table */
        rr_pc_task = rr_pc_task->next;
	}
	else {
        /* rr_rt_ttable is empty then select the IDLE task */
        ptcb = rr_idle_task;
    }

	return ptcb;
}

REGINA_STATIC rt_size rf_get_next_tick(rt_size xtick)
{
    rt_size next_tick, rrt;

    /**
     * Because Regina use D_MAX_RTOS_TICK as the mark of blocked task, so
     * the strategy of getting next tick needs to minus D_CORE_HEART_BEAT 
     * to make event get done at the right tick
     */
	if (xtick >= D_CORE_HEART_BEAT) {
        xtick -= D_CORE_HEART_BEAT;
    }

	rrt = rr_tick;
	next_tick = rrt + xtick;

    /* If result has overflowed, then get the fixed tick */
	if (next_tick < rrt) {
        next_tick = xtick - (D_MAX_RTOS_TICK - rrt);
    }

    /**
     * Avoid getting the D_MAX_RTOS_TICK as result, RTOS use it
     * as the mark of blocked task
     */
	if (next_tick == D_MAX_RTOS_TICK) {
        next_tick = 0;
    }

	return next_tick;
}

REGINA_PUBLIC void rf_schedule(void)
{
	rt_bool is_switch = rt_false;
    rt_size csr;

#if D_ENABLE_STACK_CHECK
	rf_check_stack_overflow(rr_rt_task);
#endif /* D_ENABLE_STACK_CHECK */
	/**
	 * Taks scheduler maybe called by application code
	 * RTOS needs to check if task has been locked
	 */
	if (!rr_task_lock) {
		rd_enter_critical(csr)

		if (!rd_test_rtos_invoke()) {
			is_switch = rt_true;
		}
		else { /* Function invoked by RTOS core */
			rd_reset_rtos_invoke();
		}

		if (rd_test_task_schedule()) {
			is_switch = rt_true;
			rd_reset_task_schedule();
		}
#if D_ENABLE_TASK_STAT
		rf_task_tick_stat(rr_rt_task);
#endif /*D_ENABLE_TASK_STAT*/

		/* Release the memory of rr_rt_task */
		if (rd_test_drop_task(rr_rt_task)) {
			rf_remove_tcb_from_table(rr_rt_ttable, rr_rt_task);
			rd_reset_task_magic(rr_rt_task);
			rf_free(rr_rt_task->base_addr);
			rf_free(rr_rt_task);
		}

		if (is_switch == rt_true) {
			rr_rd_task = rf_get_ready_task();
			rr_task_tick = rf_get_next_tick(D_TASK_TIME_SLICE);
		}
		else {
			/* Schedule to the last context */
			rr_rd_task = rr_pt_task;
		}
#if D_ENABLE_TASK_STAT
		rr_load_tick = rr_tick;
#endif /*D_ENABLE_TASK_STAT*/

		rd_leave_critical(csr)
		rf_switch_context();
	}
}

REGINA_PUBLIC void rf_exit(void)
{
	rt_size csr;

    if (!rr_task_lock) {
		rd_enter_critical(csr)
        rd_set_drop_task(rr_rt_task);
		rd_leave_critical(csr)
        rf_schedule();
    }
}

REGINA_PUBLIC rt_tcb* rf_create_core_task (
	rt_ushort prior, 
	rt_ushort stk_size, 
	rt_task_func pfunc, 
	rt_pvoid parg
) {
	rt_size* paddr, *pstk;
	rt_tcb* ptcb;

	paddr = (rt_size *)rf_alloc(sizeof(rt_size) * stk_size);
	rd_assert(paddr)

#if D_STACK_GROW_UP
	pstk = paddr + stk_size - 1;
#else
	pstk = paddr;
#endif /* D_STACK_GROW_UP */

	pstk = rf_setup_task_stack(pfunc, parg, pstk, rf_exit);
	ptcb = rf_create_tcb(pstk, prior, paddr);

#if D_ENABLE_STACK_CHECK
	/* Set stack label for checking stack overflow */
#if D_STACK_GROW_UP
	pstk = paddr;
	*(pstk + D_TASK_STACK_OFFSET) = D_TASK_STACK_LABEL;
#else
	pstk = paddr + stk_size - 1;
	*(pstk - D_TASK_STACK_OFFSET) = D_TASK_STACK_LABEL;
#endif /* D_STACK_GROW_UP */
	ptcb->top_addr = pstk;
#endif /* D_ENABLE_STACK_CHECK */

	return ptcb;
}

REGINA_PUBLIC void rf_create_task_inner (
	rt_task_handle* handle, 
	rt_ushort prior, 
	rt_ushort stk_size, 
	rt_task_func pfunc, 
	rt_pvoid parg
) {
	rt_tcb* ptcb;
    rt_size csr;

    rd_assert(stk_size)
	rd_assert(pfunc)

    ptcb = rf_create_core_task(prior, stk_size, pfunc, parg);
	rd_enter_critical(csr)
    rf_insert_tcb_into_table(rr_rt_ttable, ptcb);
    *handle = (rt_task_handle)ptcb;
	rd_leave_critical(csr)
}

REGINA_PUBLIC void rf_sleep(rt_size xmsec)
{
    rt_tcb_table* ptable;
	rt_size csr;
	rt_tcb* ptcb;

	/* Sleep current task if rr_task_lock is not setting */
	if (xmsec && !rr_task_lock) {
		ptcb = rr_rt_task;
		rd_enter_critical(csr)
		ptcb->wake_tick = rf_get_next_tick(xmsec);
		if (ptcb->wake_tick < rr_tick) {
            ptable = rr_sleep_ttable_of;
        }
		else {
            ptable = rr_sleep_ttable;
        }
        rf_remove_tcb_from_table(rr_rt_ttable, ptcb);
        rf_insert_tcb_into_table(ptable, ptcb);
		rd_leave_critical(csr)
		rf_schedule();
	}
	/* Block current task if rr_task_lock is setting */
	else if (xmsec && rr_task_lock) {
		rf_block_msec(xmsec);
	}
}

REGINA_PUBLIC void rf_suspend_task(rt_task_handle handle)
{
	rt_size csr;
	rt_tcb_table* ptable;
	rt_tcb* ptcb;

    ptcb = (rt_tcb *)handle;
	if (!rr_task_lock && ptcb && rd_get_task_magic(ptcb) == D_TCB_MAGIC_NUM) {
        rd_enter_critical(csr)
        ptable = (rt_tcb_table *)(ptcb->tcb_table);
        rd_assert(ptable)

        /* Set wake_tick to D_MAX_RTOS_TICK then task will be blocked */
        ptcb->wake_tick = D_MAX_RTOS_TICK;
        rf_remove_tcb_from_table(ptable, ptcb);
        rf_insert_tcb_into_table(rr_sleep_ttable, ptcb);
        rd_leave_critical(csr)

		/* Schedule to the next task if target is rr_rt_task */
        if (ptcb == rr_rt_task) {
            rf_schedule();
        }
	}
}

REGINA_PUBLIC void rf_restore_task(rt_task_handle handle)
{
    rt_tcb_table* ptable;
	rt_tcb* ptcb;
    rt_size csr;

	ptcb = (rt_tcb *)handle;
    if (ptcb && rd_get_task_magic(ptcb) == D_TCB_MAGIC_NUM) {
		rd_enter_critical(csr)
		ptable = (rt_tcb_table *)(ptcb->tcb_table);
		rd_assert(ptable)

        if (ptable != rr_rt_ttable) {
            rf_remove_tcb_from_table(ptable, ptcb);
            rf_insert_tcb_into_table(rr_rt_ttable, ptcb);
        }
		rd_leave_critical(csr)
	}
}

REGINA_PUBLIC void rf_drop_task_inner(rt_task_handle* handle)
{
	rt_size csr;
	rt_tcb_table* ptable;
	rt_tcb* ptcb;

    ptcb = (rt_tcb *)(*handle);
	if (!rr_task_lock && ptcb && rd_get_task_magic(ptcb) == D_TCB_MAGIC_NUM) {
		rd_enter_critical(csr)
        *handle = NULL;
        if (ptcb == rr_rt_task) {
            rd_set_drop_task(rr_rt_task);
			rd_leave_critical(csr)
            rf_schedule();
        }
        else {
            ptable = (rt_tcb_table *)(ptcb->tcb_table);
            rd_assert(ptable)

            rf_remove_tcb_from_table(ptable, ptcb);
            rd_reset_task_magic(ptcb);
            rf_free(ptcb->base_addr);
            rf_free(ptcb);
            rd_leave_critical(csr)
        }
    }
}

REGINA_STATIC void rf_handle_sleep_table(rt_tcb_table** ptable, rt_tcb_table** ptable_of)
{
    rt_tcb_table* ptt;
    rt_tcb* pt, *ptbk;
    rt_size csr;

    rd_assert(ptable_of)
    rd_assert(*ptable_of)
    rd_assert(ptable)
    rd_assert(*ptable)

    if (rd_test_tick_overflow()) {
        rd_enter_critical(csr)
        /* Switch sleep table when tick overflowed */
        ptt = *ptable;
        *ptable = *ptable_of;
        *ptable_of = ptt;
        /**
         * Move blocked task in rr_sleep_ttable_of 
         * to rr_sleep_ttable table
         */
        ptt = *ptable_of;
        if (ptt->count) {
            pt = ptt->head;
            do {
                ptbk = pt->next;
                /* ptt->count will update after function be called */
                rf_remove_tcb_from_table(ptt, pt);
				/* Avoid to reset the wake_tick of blocked tasks */
                if (pt->wake_tick != D_MAX_RTOS_TICK) {
                    pt->wake_tick = 0;
                }
                rf_insert_tcb_into_table(*ptable, pt);
                pt = ptbk;
            } while (ptt->count);
        }
        rd_leave_critical(csr)
    }

    ptt = *ptable;
	if (ptt->count) {
        rd_enter_critical(csr)
		pt = ptt->head;
        do {
            ptbk = pt->next;
            /* Here use '>' not '>=' to skip D_MAX_RTOS_TICK */
            if (rr_tick > pt->wake_tick) {
				/* ptt->count will update after function be called */
                rf_remove_tcb_from_table(ptt, pt);
				/* Reset target taks wake_tick */
				pt->wake_tick = 0;
			    rf_insert_tcb_into_table(rr_rt_ttable, pt);
            }
			else { /* Table has sorted in an ascending order */
				break;
			}
            pt = ptbk;
        } while (ptt->count);
        rd_leave_critical(csr)
	}
}

REGINA_PUBLIC void rf_handle_tick_isr(void)
{
	rt_size csr, hdl_mode = 0;

	if (rd_test_rtos_running()) {
#if D_ENABLE_HEART_BEAT_HOOK
		rf_heart_beat_hook();
#endif /* D_ENABLE_HEART_BEAT_HOOK */

#if D_ENABLE_STACK_CHECK
		rf_check_stack_overflow(rr_rt_task);
#endif /* D_ENABLE_STACK_CHECK */

        rd_enter_critical(csr)
		rr_tick++;
		rd_leave_critical(csr)

		if (!(rr_tick % D_CORE_HEART_BEAT)) {
			hdl_mode = 1;
		}
		else if (rd_test_task_schedule()) {
			hdl_mode = 2;
		}

		if (hdl_mode && !rr_task_lock && rr_rt_task != rr_tick_task) {
			rd_enter_critical(csr)
#if D_ENABLE_TASK_STAT
			rf_task_tick_stat(rr_rt_task);
#endif /*D_ENABLE_TASK_STAT*/

			if (hdl_mode == 1) {
				rr_pt_task = rr_rt_task;
				rr_rd_task = rr_tick_task;
			}
			else {
				rr_rd_task = rf_get_ready_task();
				rr_task_tick = rf_get_next_tick(D_TASK_TIME_SLICE);
				rd_reset_task_schedule();
			}
#if D_ENABLE_TASK_STAT
			rr_load_tick = rr_tick;
#endif /*D_ENABLE_TASK_STAT*/

			rd_leave_critical(csr)
			rf_switch_context();
		}
	}
}

#if D_ENABLE_SOFT_TIMER

/* Definitions of operations of RTOS soft timer flags */
#define rd_get_timer_magic(timer) (((timer)->timer_ctl & D_TIMER_MAGIC_MASK) >> 0)
#define rd_reset_timer_magic(timer) rd_reset_bits((timer)->timer_ctl, D_TIMER_MAGIC_MASK)
#define rd_set_timer_magic(timer, magic) rd_set_bits((timer)->timer_ctl, D_TIMER_MAGIC_MASK, magic)

#define rd_test_timer_periodic(timer) rd_test_bit((timer)->timer_ctl, D_TIMER_PERIODIC_MASK)
#define rd_reset_timer_periodic(timer) rd_reset_bit((timer)->timer_ctl, D_TIMER_PERIODIC_MASK)
#define rd_set_timer_periodic(timer) rd_set_bit((timer)->timer_ctl, D_TIMER_PERIODIC_MASK)

#define rd_test_timer_running(timer) rd_test_bit((timer)->timer_ctl, D_TIMER_RUNNING_MASK)
#define rd_reset_timer_running(timer) rd_reset_bit((timer)->timer_ctl, D_TIMER_RUNNING_MASK)
#define rd_set_timer_running(timer) rd_set_bit((timer)->timer_ctl, D_TIMER_RUNNING_MASK)

REGINA_STATIC rt_timer* rf_create_core_timer (
	rt_size period, 
	rt_timer_func pfunc, 
	rt_pvoid parg, 
	rt_bool is_periodic
) {
	rt_timer* ptimer;

	ptimer = (rt_timer *)rf_alloc(sizeof(rt_timer));
	rd_assert(ptimer)

	rd_set_timer_magic(ptimer, D_TIM_MAGIC_NUM);
	rd_set_timer_running(ptimer);

	if (is_periodic == rt_true) {
		rd_set_timer_periodic(ptimer);
	}
	else {
		rd_reset_timer_periodic(ptimer);
	}
	ptimer->period = period;
	ptimer->pfunc = pfunc;
	ptimer->parg = parg;
	ptimer->wake_tick = 0;
	ptimer->tim_table = NULL;

	ptimer->last = NULL;
	ptimer->next = NULL;

	return ptimer;
}

REGINA_STATIC rt_timer_table* rf_create_timer_table(void)
{
	rt_timer_table* ptable;

	ptable = (rt_timer_table *)rf_alloc(sizeof(rt_timer_table));
	rd_assert(ptable)

	ptable->count = 0;
	ptable->head = NULL;

	return ptable;
}

REGINA_STATIC void rf_insert_timer_into_table(rt_timer_table* ptable, rt_timer* ptimer)
{
    rt_timer* pt, *list;

    rd_assert(ptable)
    rd_assert(ptimer)

    if (!ptable->count) {
        rd_assert(!ptable->head)

        ptable->head = ptimer;
        ptimer->next = ptimer;
        ptimer->last = ptimer;
    }
    else { /* if (ptable->count) */
        rd_assert(ptable->head)

        list = ptable->head;
		pt = NULL;
		do {
            if (ptimer->wake_tick < list->wake_tick) {
				pt = list;
				break;
			}
			list = list->next;
		} while (list != ptable->head);

		if (pt == NULL) {
			pt = ptable->head;
		}
		else if (pt == ptable->head) {
			ptable->head = ptimer;
		}

		ptimer->next = pt;
        ptimer->last = pt->last;
        pt->last = ptimer;
        ptimer->last->next = ptimer;
    }
    ptable->count++;
	ptimer->tim_table = ptable;
}

REGINA_STATIC void rf_remove_timer_from_table(rt_timer_table* ptable, rt_timer* ptimer)
{
    rd_assert(ptable)
    rd_assert(ptimer)

	rd_assert((rt_pvoid)ptable == ptimer->tim_table)
	rd_assert(ptable->count)
    if (ptable->count == 1) {
        rd_assert(ptimer == ptable->head)

        ptable->head = NULL;
    }
    else if (ptable->count > 1) {
        if (ptimer == ptable->head) {
            ptable->head = ptimer->next;
        }
    }
	ptimer->last->next = ptimer->next;
	ptimer->next->last = ptimer->last;
	ptimer->last = NULL;
	ptimer->next = NULL;

	rd_assert(ptable->count)
	ptable->count--;
    ptimer->tim_table = NULL;
}

REGINA_PUBLIC void rf_set_timer_arg(rt_timer_handle handle, rt_pvoid parg)
{
	rt_timer* ptimer;
	rt_size csr;

	ptimer = (rt_timer *)handle;
	if (ptimer && rd_get_timer_magic(ptimer) == D_TIM_MAGIC_NUM) {
		rd_enter_critical(csr)
		ptimer->parg = parg;
		rd_leave_critical(csr)
	}
}

REGINA_PUBLIC void rf_set_timer_period(rt_timer_handle handle, rt_size period)
{
	rt_timer* ptimer;
	rt_size csr;

	ptimer = (rt_timer *)handle;
	if (ptimer && rd_get_timer_magic(ptimer) == D_TIM_MAGIC_NUM) {
		rd_enter_critical(csr)
		ptimer->period = period;
		rd_leave_critical(csr)
	}
}

REGINA_PUBLIC void rf_start_timer(rt_timer_handle handle)
{
	rt_timer_table* ptable;
	rt_timer* ptimer;
	rt_size csr;

	ptimer = (rt_timer *)handle;
	if (ptimer && rd_get_timer_magic(ptimer) == D_TIM_MAGIC_NUM) {
		/* Function start or restart the timer */
		rd_enter_critical(csr)
		ptable = (rt_timer_table *)(ptimer->tim_table);
		rd_assert(ptable)

		rf_remove_timer_from_table(ptable, ptimer);
		ptimer->wake_tick = rf_get_next_tick(ptimer->period);
		if (ptimer->wake_tick < rr_tick) {
			ptable = rr_timer_table_of;
		}
		else {
			ptable = rr_timer_table;
		}
		rd_set_timer_running(ptimer);
		rf_insert_timer_into_table(ptable, ptimer);
		rd_leave_critical(csr)
	}
}

REGINA_PUBLIC void rf_stop_timer(rt_timer_handle handle)
{
	rt_timer_table* ptable;
	rt_timer* ptimer;
	rt_size csr;

	ptimer = (rt_timer *)handle;
	if (ptimer && rd_get_timer_magic(ptimer) == D_TIM_MAGIC_NUM) {
		if (rd_test_timer_running(ptimer)) {
			rd_enter_critical(csr)
			ptable = (rt_timer_table *)(ptimer->tim_table);
			rd_assert(ptable)

			rf_remove_timer_from_table(ptable, ptimer);
			ptimer->wake_tick = D_MAX_RTOS_TICK;
			rd_reset_timer_running(ptimer);
			rf_insert_timer_into_table(rr_timer_table, ptimer);
			rd_leave_critical(csr)
		}
	}
}

REGINA_PUBLIC void rf_drop_timer_inner(rt_timer_handle* handle)
{
	rt_timer_table* ptable;
	rt_timer* ptimer;
	rt_size csr;

	ptimer = (rt_timer *)(*handle);
	if (ptimer && rd_get_timer_magic(ptimer) == D_TIM_MAGIC_NUM) {
		rd_enter_critical(csr)
		*handle = NULL;
		ptable = (rt_timer_table *)(ptimer->tim_table);
		rd_assert(ptable)

		rf_remove_timer_from_table(ptable, ptimer);
		rd_reset_timer_magic(ptimer);
		rf_free(ptimer);
		rd_leave_critical(csr)
	}
}

REGINA_PUBLIC void rf_create_timer_node (
	rt_timer_handle* handle, 
	rt_size period, 
	rt_timer_func pfunc, 
	rt_pvoid parg, 
	rt_bool is_periodic
) {
	rt_timer_table* ptable;
	rt_timer* ptimer;
	rt_size csr;

	rd_assert(period)
	rd_assert(pfunc)

	ptimer = rf_create_core_timer(period, pfunc, parg, is_periodic);
	/* Function default to start timer */
	rd_enter_critical(csr)
	ptimer->wake_tick = rf_get_next_tick(ptimer->period);
	if (ptimer->wake_tick < rr_tick) {
		ptable = rr_timer_table_of;
	}
	else {
		ptable = rr_timer_table;
	}
	rf_insert_timer_into_table(ptable, ptimer);
	*handle = (rt_timer_handle)ptimer;
	rd_leave_critical(csr)
}

REGINA_PUBLIC void rf_create_timer_inner (
	rt_timer_handle* handle, 
	rt_size period, 
	rt_timer_func pfunc, 
	rt_pvoid parg
) {
	rf_create_timer_node(handle, period, pfunc, parg, rt_true);
}

REGINA_PUBLIC void rf_create_single_timer_inner (
	rt_timer_handle* handle, 
	rt_size period, 
	rt_timer_func pfunc, 
	rt_pvoid parg
) {
	rf_create_timer_node(handle, period, pfunc, parg, rt_false);
}

REGINA_STATIC void rf_handle_timer_table(rt_timer_table** ptable, rt_timer_table** ptable_of)
{
    rt_timer_table* ptt, *pttbk;
    rt_timer* pt, *ptbk;
    rt_size csr, it;

	/* Buffer of synchronous timer execution */
	rt_timer* ptimers[8];
	rt_timer_func pfuncs[8];
	rt_pvoid pargs[8];
	rt_size index = 0;

    rd_assert(ptable_of)
    rd_assert(*ptable_of)
    rd_assert(ptable)
    rd_assert(*ptable)

    if (rd_test_tick_overflow()) {
        rd_enter_critical(csr)
		/**
         * Move blocked task in rr_timer_table
         * to rr_timer_table_of table
         */
        ptt = *ptable;
        *ptable = *ptable_of;
        *ptable_of = ptt;

        ptt = *ptable_of;
        if (ptt->count) {
            pt = ptt->head;
            do {
                ptbk = pt->next;
                rf_remove_timer_from_table(ptt, pt);
                if (pt->wake_tick != D_MAX_RTOS_TICK) {
                    pt->wake_tick = 0;
                }
                rf_insert_timer_into_table(*ptable, pt);
                pt = ptbk;
            } while (ptt->count);
        }
        rd_leave_critical(csr)
    }

    ptt = *ptable;
	if (ptt->count) {
        rd_enter_critical(csr)
		pt = ptt->head;
		do {
            if (rr_tick > pt->wake_tick) {
				/* Record timer function and argument */
				ptimers[index] = pt;
				pfuncs[index] = pt->pfunc;
				pargs[index] = pt->parg;
				index++;
				if (index >= 8) {
					break;
				}
            }
			else { /* Table has sorted in an ascending order */
				break;
			}
			pt = pt->next;
        } while (pt != ptt->head);

		/* Rearrange the selected timer wake_tick */
		for (it = 0; it < index; it++) {
			pt = ptimers[it];
			rf_remove_timer_from_table(ptt, pt);
			pttbk = ptt;
			if (!rd_test_timer_periodic(pt)) {
				pt->wake_tick = D_MAX_RTOS_TICK;
			}
			else {
				pt->wake_tick = rf_get_next_tick(pt->period);
				if (pt->wake_tick < rr_tick) {
					pttbk = *ptable_of;
				}
			}
			rf_insert_timer_into_table(pttbk, pt);
		}
		rd_leave_critical(csr)
		/**
		 * RTOS use buffer to avoid invoking 
		 * functions under critical
		 */
		while (index) {
			index--;
			pfuncs[index](pargs[index]);
		}
	}
}
#endif /* D_ENABLE_SOFT_TIMER */

#if D_ENABLE_SEMAPHORE || D_ENABLE_MESSAGE_QUEUE
REGINA_STATIC rt_tcb* rf_get_prior_task_from_table(rt_tcb_table* ptable)
{
	rt_tcb* result = NULL, *ptcb;
	rt_ushort prior;

	rd_assert(ptable)

	if (ptable->count) {
		ptcb = ptable->head;
		prior = ptcb->prior;
		result = ptcb;
		do {
			if (ptcb->prior < prior) {
				prior = ptcb->prior;
				result = ptcb;
			}
			ptcb = ptcb->next;
		} while (ptcb != ptable->head);
	}
	return result;
}

REGINA_STATIC void rf_clean_tcb_table(rt_tcb_table* psrc, rt_tcb_table* pdst)
{
	rt_tcb* pt, *ptbk;

	rd_assert(psrc)
	rd_assert(pdst)
	rd_assert(psrc != pdst)

	if (psrc->count) {
		pt = psrc->head;
		do {
			ptbk = pt->next;
			rf_remove_tcb_from_table(psrc, pt);
			rf_insert_tcb_into_table(pdst, pt);
			pt = ptbk;
		} while (psrc->count);
	}
}
#endif /* D_ENABLE_SEMAPHORE || D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_MESSAGE_QUEUE

/* Definitions of operations of RTOS message queue flags */
#define rd_get_msgq_magic(msgq) (((msgq)->msgq_ctl & D_MSGQ_MAGIC_MASK) >> 0)
#define rd_reset_msgq_magic(msgq) rd_reset_bits((msgq)->msgq_ctl, D_MSGQ_MAGIC_MASK)
#define rd_set_msgq_magic(msgq, magic) rd_set_bits((msgq)->msgq_ctl, D_MSGQ_MAGIC_MASK, magic)

#define rd_test_msgq_empty(msgq) rd_test_bit((msgq)->msgq_ctl, D_MSGQ_EMPTY_MASK)
#define rd_reset_msgq_empty(msgq) rd_reset_bit((msgq)->msgq_ctl, D_MSGQ_EMPTY_MASK)
#define rd_set_msgq_empty(msgq) rd_set_bit((msgq)->msgq_ctl, D_MSGQ_EMPTY_MASK)

#define rd_test_msgq_full(msgq) rd_test_bit((msgq)->msgq_ctl, D_MSGQ_FULL_MASK)
#define rd_reset_msgq_full(msgq) rd_reset_bit((msgq)->msgq_ctl, D_MSGQ_FULL_MASK)
#define rd_set_msgq_full(msgq) rd_set_bit((msgq)->msgq_ctl, D_MSGQ_FULL_MASK)

REGINA_STATIC rt_queue* rf_create_queue(void)
{
	rt_queue* queue;

	queue = (rt_queue *)rf_alloc(sizeof(rt_queue));
	rd_assert(queue)

	queue->front = 0;
	queue->rear = 0;

	return queue;
}

REGINA_STATIC rt_msgq* rf_create_core_msgq(void)
{
	rt_msgq* msgq;

	msgq = (rt_msgq *)rf_alloc(sizeof(rt_msgq));
	rd_assert(msgq)

	rd_set_msgq_magic(msgq, D_MSG_MAGIC_NUM);
	rd_set_msgq_empty(msgq);
	rd_reset_msgq_full(msgq);

	msgq->queue = rf_create_queue();
	msgq->read_list_of = rf_create_tcb_table();
	msgq->read_list = rf_create_tcb_table();
	msgq->write_list_of = rf_create_tcb_table();
	msgq->write_list = rf_create_tcb_table();

	msgq->next = NULL;
	msgq->last = NULL;

	return msgq;
}

REGINA_STATIC rt_msgq_table* rf_create_msgq_table(void)
{
	rt_msgq_table* ptable;

	ptable = (rt_msgq_table *)rf_alloc(sizeof(rt_msgq_table));
	rd_assert(ptable)

	ptable->count = 0;
	ptable->head = NULL;

	return ptable;
}

REGINA_STATIC void rf_push_msg_into_msgq(rt_msgq* msgq, rt_msg* msg)
{
	rt_queue* q;

	rd_assert(msgq)
	rd_assert(msg)

	q = msgq->queue;
	if (((q->rear + 1) % D_MESSAGE_QUEUE_LENGTH) != q->front) {
		q->rear = (q->rear + 1) % D_MESSAGE_QUEUE_LENGTH;
		q->data[q->rear] = *msg;

		/* The message queue is full then set mask */
		if (((q->rear + 1) % D_MESSAGE_QUEUE_LENGTH) == q->front) {
			rd_set_msgq_full(msgq);
		}
		rd_reset_msgq_empty(msgq);
	}
}

REGINA_STATIC void rf_pop_msg_from_msgq(rt_msgq* msgq, rt_msg* msg)
{
	rt_queue* q;

	rd_assert(msgq)
	rd_assert(msg)

	q = msgq->queue;
	if (q->front != q->rear) {
		q->front = (q->front + 1) % D_MESSAGE_QUEUE_LENGTH;
		*msg = q->data[q->front];

		/* The message queue is empty then set mask */
		if (q->front == q->rear) {
			rd_set_msgq_empty(msgq);
		}
		rd_reset_msgq_full(msgq);
	}
}

REGINA_STATIC void rf_insert_msgq_into_table(rt_msgq_table* ptable, rt_msgq* msgq)
{
    rt_msgq* pt;

    rd_assert(ptable)
    rd_assert(msgq)

    if (!ptable->count) {
        rd_assert(!ptable->head)

        ptable->head = msgq;
        msgq->next = msgq;
        msgq->last = msgq;
    }
    else { /* if (ptable->count) */
        rd_assert(ptable->head)

		/* Always insert to table tail */
		pt = ptable->head;

		msgq->next = pt;
        msgq->last = pt->last;
        pt->last = msgq;
        msgq->last->next = msgq;
    }
    ptable->count++;
}

REGINA_STATIC void rf_remove_msgq_from_table(rt_msgq_table* ptable, rt_msgq* msgq)
{
    rd_assert(ptable)
    rd_assert(msgq)

	rd_assert(ptable->count)
    if (ptable->count == 1) {
        rd_assert(msgq == ptable->head)

        ptable->head = NULL;
    }
    else if (ptable->count > 1) {
        if (msgq == ptable->head) {
            ptable->head = msgq->next;
        }
    }
	msgq->last->next = msgq->next;
	msgq->next->last = msgq->last;
	msgq->last = NULL;
	msgq->next = NULL;

	rd_assert(ptable->count)
	ptable->count--;
}

REGINA_STATIC void rf_restore_task_from_msgq(rt_msgq* msgq, rt_operate opt)
{
	rt_tcb_table* ptt;
	rt_tcb* pt;
	/**
	 * Function restores task from read or write blocked list 
	 * of message queue, the feasibility of restoring task
	 * must be checked before invoking
	 */
	if (opt == rt_read) {
		ptt = msgq->read_list;
	}
	else {
		ptt = msgq->write_list;
	}
	pt = rf_get_prior_task_from_table(ptt);
	rd_assert(pt)

	rf_remove_tcb_from_table(ptt, pt);
	if (opt == rt_read) {
		rf_pop_msg_from_msgq(msgq, &(pt->tcb_msg));
	}
	else {
		rf_push_msg_into_msgq(msgq, &(pt->tcb_msg));
	}
	rd_set_wake_result(pt);
	rf_insert_tcb_into_table(rr_rt_ttable, pt);
}

REGINA_STATIC void rf_move_task_to_msgq(rt_msgq* msgq, rt_size xmsec, rt_operate opt)
{
	rt_tcb_table* ptt;
	rt_tcb* pt;
	rt_size csr;
	/**
	 * Function move task to read or write blocked list 
	 * of message queue, if rr_task_lock is set then
	 * function will return immediately
	 */
	if (xmsec && !rr_task_lock) {
		rd_enter_critical(csr)
		if (opt == rt_read) {
			ptt = msgq->read_list;
		}
		else {
			ptt = msgq->write_list;
		}
		pt = rr_rt_task;
		if (xmsec == D_MAX_RTOS_TICK) {
			pt->wake_tick = xmsec;
		}
		else {
			pt->wake_tick = rf_get_next_tick(xmsec);
			if (pt->wake_tick < rr_tick) {
				if (opt == rt_read) {
					ptt = msgq->read_list_of;
				}
				else {
					ptt = msgq->write_list_of;
				}
			}
		}
		rf_remove_tcb_from_table(rr_rt_ttable, pt);
		rf_insert_tcb_into_table(ptt, pt);
		rd_leave_critical(csr)
		rf_schedule();
	}
}

REGINA_PUBLIC void rf_create_msgq_inner(rt_msgq_handle* handle)
{
	rt_msgq* msgq;
	rt_size csr;

	msgq = rf_create_core_msgq();
	rd_enter_critical(csr)
	rf_insert_msgq_into_table(rr_msgq_table, msgq);
	*handle = (rt_msgq_handle)msgq;
	rd_leave_critical(csr)
}

REGINA_PUBLIC void rf_drop_msgq_inner(rt_msgq_handle* handle)
{
	rt_msgq* msgq;
	rt_size csr;

	msgq = (rt_msgq *)(*handle);
	if (msgq && rd_get_msgq_magic(msgq) == D_MSG_MAGIC_NUM) {
		rd_enter_critical(csr)
		*handle = NULL;
		rf_remove_msgq_from_table(rr_msgq_table, msgq);

		rf_clean_tcb_table(msgq->write_list, rr_sleep_ttable);
		rf_clean_tcb_table(msgq->write_list_of, rr_sleep_ttable_of);
		rf_clean_tcb_table(msgq->read_list, rr_sleep_ttable);
		rf_clean_tcb_table(msgq->read_list_of, rr_sleep_ttable_of);

		rd_reset_msgq_magic(msgq);
		rf_free(msgq->queue);
		rf_free(msgq->read_list);
		rf_free(msgq->read_list_of);
		rf_free(msgq->write_list);
		rf_free(msgq->write_list_of);
		rf_free(msgq);
		rd_leave_critical(csr)
	}
}

REGINA_PUBLIC rt_result rf_send_message(rt_msgq_handle handle, rt_msg* msg, rt_size xmsec)
{
	rt_tcb* ptcb;
	rt_msgq* msgq;
	rt_size csr;

	rd_assert(msg)

	ptcb = rr_rt_task;
	/* Reset the result of sending message to message queue */
	rd_reset_wake_result(ptcb);

	msgq = (rt_msgq *)handle;
	if (msgq && rd_get_msgq_magic(msgq) == D_MSG_MAGIC_NUM) {
		if (!rd_test_msgq_full(msgq)) {
			rd_enter_critical(csr)
			rf_push_msg_into_msgq(msgq, msg);
			if (msgq->read_list->count) {
				rf_restore_task_from_msgq(msgq, rt_read);
			}
			rd_set_wake_result(ptcb);
			rd_leave_critical(csr)
		}
		else {
			ptcb->tcb_msg = *msg;
			rf_move_task_to_msgq(msgq, xmsec, rt_write);
		}
	}
	return (rd_test_wake_result(ptcb) ? rt_true : rt_false);
}

REGINA_PUBLIC rt_result rf_send_message_isr(rt_msgq_handle handle, rt_msg* msg)
{
	rt_result result = rt_false;
	rt_msgq* msgq;
	rt_size csr;

	rd_assert(msg)

	msgq = (rt_msgq *)handle;
	if (msgq && rd_get_msgq_magic(msgq) == D_MSG_MAGIC_NUM && !rd_test_msgq_full(msgq)) {
		rd_enter_critical(csr)
		rf_push_msg_into_msgq(msgq, msg);
		if (msgq->read_list->count) {
			rf_restore_task_from_msgq(msgq, rt_read);
		}
		result = rt_true;
		rd_leave_critical(csr)
	}
	return result;
}

REGINA_PUBLIC rt_result rf_receive_message(rt_msgq_handle handle, rt_msg* msg, rt_size xmsec)
{
	rt_msgq* msgq;
	rt_tcb* ptcb;
	rt_size csr;

	rd_assert(msg)

	ptcb = rr_rt_task;
	/* Reset the result of sending message to message queue */
	rd_reset_wake_result(ptcb);

	msgq = (rt_msgq *)handle;
	if (msgq && rd_get_msgq_magic(msgq) == D_MSG_MAGIC_NUM) {
		if (!rd_test_msgq_empty(msgq)) {
			rd_enter_critical(csr)
			rf_pop_msg_from_msgq(msgq, msg);
			if (msgq->write_list->count) {
				rf_restore_task_from_msgq(msgq, rt_write);
			}
			rd_set_wake_result(ptcb);
			rd_leave_critical(csr)
		}
		else {
			rf_move_task_to_msgq(msgq, xmsec, rt_read);
			*msg = ptcb->tcb_msg;
		}
	}
	return (rd_test_wake_result(ptcb) ? rt_true : rt_false);
}

REGINA_STATIC void rf_handle_msgq_table(rt_msgq_table* ptable) 
{
	rt_msgq* msgq;
	rt_size csr;

	rd_assert(ptable)

	if (ptable->count) {
		rd_enter_critical(csr)
		msgq = ptable->head;
		do {
			rf_handle_sleep_table(&(msgq->read_list), &(msgq->read_list_of));
			rf_handle_sleep_table(&(msgq->write_list), &(msgq->write_list_of));
			msgq = msgq->next;
		} while (msgq != ptable->head);
		rd_leave_critical(csr)
	}
}
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE

/* Definitions of operations of RTOS semaphore flags */
#define rd_get_mutex_magic(mutex) (((mutex)->mutex_ctl & D_MUTEX_MAGIC_MASK) >> 0)
#define rd_reset_mutex_magic(mutex) rd_reset_bits((mutex)->mutex_ctl, D_MUTEX_MAGIC_MASK)
#define rd_set_mutex_magic(mutex, magic) rd_set_bits((mutex)->mutex_ctl, D_MUTEX_MAGIC_MASK, magic)

#define rd_test_mutex_void(mutex) rd_test_bit((mutex)->mutex_ctl, D_MUTEX_VOID_MASK)
#define rd_reset_mutex_void(mutex) rd_reset_bit((mutex)->mutex_ctl, D_MUTEX_VOID_MASK)
#define rd_set_mutex_void(mutex) rd_set_bit((mutex)->mutex_ctl, D_MUTEX_VOID_MASK)

#define rd_test_mutex_hold(mutex) rd_test_bit((mutex)->mutex_ctl, D_MUTEX_HOLD_MASK)
#define rd_reset_mutex_hold(mutex) rd_reset_bit((mutex)->mutex_ctl, D_MUTEX_HOLD_MASK)
#define rd_set_mutex_hold(mutex) rd_set_bit((mutex)->mutex_ctl, D_MUTEX_HOLD_MASK)

REGINA_STATIC rt_mutex* rf_create_core_mutex(rt_bool is_void)
{
	rt_mutex* mutex;

	mutex = (rt_mutex *)rf_alloc(sizeof(rt_mutex));
	rd_assert(mutex)

	rd_set_mutex_magic(mutex, D_SEM_MAGIC_NUM);
	if (is_void == rt_true) {
		rd_set_mutex_hold(mutex);
		rd_set_mutex_void(mutex);
	}
	else {
		rd_reset_mutex_hold(mutex);
		rd_reset_mutex_void(mutex);
	}
	mutex->request_list = rf_create_tcb_table();
	mutex->request_list_of = rf_create_tcb_table();
	mutex->holder = NULL;

	mutex->next = NULL;
	mutex->last = NULL;

	return mutex;
}

REGINA_STATIC rt_mutex_table* rf_create_mutex_table(void)
{
	rt_mutex_table* ptable;

	ptable = (rt_mutex_table *)rf_alloc(sizeof(rt_mutex_table));
	rd_assert(ptable)

	ptable->count = 0;
	ptable->head = NULL;

	return ptable;
}

REGINA_STATIC void rf_insert_mutex_into_table(rt_mutex_table* ptable, rt_mutex* mutex)
{
    rt_mutex* pt;

    rd_assert(ptable)
    rd_assert(mutex)

    if (!ptable->count) {
        rd_assert(!ptable->head)

        ptable->head = mutex;
        mutex->next = mutex;
        mutex->last = mutex;
    }
    else { /* if (ptable->count) */
        rd_assert(ptable->head)

		/* Always insert to table tail */
		pt = ptable->head;

		mutex->next = pt;
        mutex->last = pt->last;
        pt->last = mutex;
        mutex->last->next = mutex;
    }
    ptable->count++;
}

REGINA_STATIC void rf_remove_mutex_from_table(rt_mutex_table* ptable, rt_mutex* mutex)
{
    rd_assert(ptable)
    rd_assert(mutex)

	rd_assert(ptable->count)
    if (ptable->count == 1) {
        rd_assert(mutex == ptable->head)

        ptable->head = NULL;
    }
    else if (ptable->count > 1) {
        if (mutex == ptable->head) {
            ptable->head = mutex->next;
        }
    }
	mutex->last->next = mutex->next;
	mutex->next->last = mutex->last;
	mutex->last = NULL;
	mutex->next = NULL;

	rd_assert(ptable->count)
	ptable->count--;
}

REGINA_STATIC void rf_adjust_task_prior(rt_tcb* ptcb, rt_ushort prior)
{
	rt_tcb_table* ptt;

	if (ptcb->prior != prior) {
		ptt = (rt_tcb_table *)(ptcb->tcb_table);
		rd_assert(ptt)

		/* Only rr_rt_ttable is sorted by task priority */
		if (ptt != rr_rt_ttable) {
			ptcb->prior = prior;
		}
		else {
			rf_remove_tcb_from_table(ptt, ptcb);
			ptcb->prior = prior;
			rf_insert_tcb_into_table(ptt, ptcb);
		}
	}
}

REGINA_STATIC void rf_create_mutex_node(rt_mutex_handle* handle, rt_bool is_void)
{
	rt_mutex* mutex;
	rt_size csr;

	mutex = rf_create_core_mutex(is_void);
	rd_enter_critical(csr)
	rf_insert_mutex_into_table(rr_mutex_table, mutex);
	*handle = (rt_mutex_handle)mutex;
	rd_leave_critical(csr)
}

REGINA_PUBLIC void rf_create_mutex_inner(rt_mutex_handle* handle) {
	rf_create_mutex_node(handle, rt_false);
}

REGINA_PUBLIC void rf_create_lock_inner(rt_mutex_handle* handle) {
	rf_create_mutex_node(handle, rt_true);
}

REGINA_PUBLIC void rf_drop_mutex_inner(rt_mutex_handle* handle)
{
	rt_mutex* mutex;
	rt_tcb* ptcb;
	rt_size csr;

	mutex = (rt_mutex *)(*handle);
	if (mutex && rd_get_mutex_magic(mutex) == D_SEM_MAGIC_NUM) {
		rd_enter_critical(csr)
		*handle = NULL;
		rf_remove_mutex_from_table(rr_mutex_table, mutex);

		rf_clean_tcb_table(mutex->request_list_of, rr_sleep_ttable_of);
		rf_clean_tcb_table(mutex->request_list, rr_sleep_ttable);

		/* Adjust task priority if mutex has been held */
		if (rd_test_mutex_hold(mutex) && !rd_test_mutex_void(mutex)) {
			ptcb = mutex->holder;
			rd_assert(ptcb)

			rf_adjust_task_prior(ptcb, ptcb->tcb_prior);
		}
		rd_reset_mutex_magic(mutex);
		rf_free(mutex->request_list);
		rf_free(mutex->request_list_of);
		rf_free(mutex);
		rd_leave_critical(csr)
	}
}

REGINA_STATIC void rf_restore_task_from_mutex(rt_mutex* mutex)
{
	rt_tcb_table* ptt;
	rt_tcb* pt;
	/**
	 * Function restores task from request blocked list of mutex
	 * The feasibility of restoring task must be checked
	 */
	ptt = mutex->request_list;
	pt = rf_get_prior_task_from_table(ptt);
	rd_assert(pt)

	rf_remove_tcb_from_table(ptt, pt);
	rd_set_mutex_hold(mutex);
	if (!rd_test_mutex_void(mutex)) {
		mutex->holder = pt;
	}
	rd_set_wake_result(pt);
	rf_insert_tcb_into_table(rr_rt_ttable, pt);
}

REGINA_STATIC void rf_move_task_to_mutex(rt_mutex* mutex, rt_size xmsec)
{
	rt_tcb_table* ptt;
	rt_tcb* pt;
	rt_size csr;
	/**
	 * Function move task to request blocked list 
	 * of mutex, if rr_task_lock is set then
	 * function will return immediately
	 */
	if (xmsec && !rr_task_lock) {
		rd_enter_critical(csr)
		ptt = mutex->request_list;
		pt = rr_rt_task;
		if (xmsec == D_MAX_RTOS_TICK) {
			pt->wake_tick = xmsec;
		}
		else {
			pt->wake_tick = rf_get_next_tick(xmsec);
			if (pt->wake_tick < rr_tick) {
				ptt = mutex->request_list_of;
			}
		}
		rf_remove_tcb_from_table(rr_rt_ttable, pt);
		rf_insert_tcb_into_table(ptt, pt);
		rd_leave_critical(csr)
		rf_schedule();
	}
}

REGINA_PUBLIC rt_result rf_release_mutex (rt_mutex_handle handle)
{
	rt_mutex* mutex;
	rt_tcb* ptcb;
	rt_size csr;

	ptcb = rr_rt_task;
	/* Reset the result of sending message to message queue */
	rd_reset_wake_result(ptcb);

	mutex = (rt_mutex *)handle;
	if (mutex && rd_get_mutex_magic(mutex) == D_SEM_MAGIC_NUM) {
		if (rd_test_mutex_hold(mutex)) {
			rd_enter_critical(csr)
			if (!rd_test_mutex_void(mutex)) {
				if (ptcb == mutex->holder) {
					rf_adjust_task_prior(ptcb, ptcb->tcb_prior);
					rd_reset_mutex_hold(mutex);
					mutex->holder = NULL;
					rd_set_wake_result(ptcb);
				}
			}
			else {
				rd_reset_mutex_hold(mutex);
				rd_set_wake_result(ptcb);
			}
			if (rd_test_wake_result(ptcb) && mutex->request_list->count) {
				rf_restore_task_from_mutex(mutex);
			}
			rd_leave_critical(csr)
		}
	}
	return (rd_test_wake_result(ptcb) ? rt_true : rt_false);
}

REGINA_PUBLIC rt_result rf_release_lock_isr(rt_lock_handle handle)
{
	rt_result result = rt_false;
	rt_mutex* mutex;
	rt_size csr;

	mutex = (rt_mutex *)handle;
	if (mutex && rd_get_mutex_magic(mutex) == D_SEM_MAGIC_NUM) {
		if (rd_test_mutex_hold(mutex) && rd_test_mutex_void(mutex)) {
			rd_enter_critical(csr)
			rd_reset_mutex_hold(mutex);
			if (mutex->request_list->count) {
				rf_restore_task_from_mutex(mutex);
			}
			result = rt_true;
			rd_leave_critical(csr)
		}
	}
	return result;
}

REGINA_PUBLIC rt_result rf_request_mutex(rt_mutex_handle handle, rt_size xmsec)
{
	rt_mutex* mutex;
	rt_tcb* ptcb;
	rt_size csr;

	ptcb = rr_rt_task;
	/* Reset the result of sending message to message queue */
	rd_reset_wake_result(ptcb);

	mutex = (rt_mutex *)handle;
	if (mutex && rd_get_mutex_magic(mutex) == D_SEM_MAGIC_NUM) {
		/* The mutex or lock request may cause deadlock */
		rd_enter_critical(csr)
		if (!rd_test_mutex_hold(mutex)) {
			rd_set_mutex_hold(mutex);
			if (!rd_test_mutex_void(mutex)) {
				mutex->holder = ptcb;
			}
			rd_set_wake_result(ptcb);
			rd_leave_critical(csr)
		}
		else {
			rd_assert(mutex->holder)

			if (!rd_test_mutex_void(mutex) && ptcb->prior < mutex->holder->prior) {
				rf_adjust_task_prior(mutex->holder, ptcb->prior);
			}
			rd_leave_critical(csr)
			rf_move_task_to_mutex(mutex, xmsec);
		}
	}
	return (rd_test_wake_result(ptcb) ? rt_true : rt_false);
}

REGINA_STATIC void rf_handle_mutex_table(rt_mutex_table* ptable) 
{
	rt_mutex* mutex;
	rt_size csr;

	rd_assert(ptable)

	if (ptable->count) {
		rd_enter_critical(csr)
		mutex = ptable->head;
		do {
			rf_handle_sleep_table(&(mutex->request_list), &(mutex->request_list_of));
			mutex = mutex->next;
		} while (mutex != ptable->head);
		rd_leave_critical(csr)
	}
}
#endif /* D_ENABLE_SEMAPHORE */

REGINA_STATIC void rf_idle_task(rt_pvoid parg)
{
_TASK_LOOP:
#if D_ENABLE_FREE_TASK_HOOK
    rf_free_stack_hook();
#endif /* D_ENABLE_FREE_TASK_HOOK */
    goto _TASK_LOOP;
}

REGINA_STATIC void rf_tick_task(rt_pvoid parg)
{
	rt_size csr;

	/* Handle events of all tasks sleep */
_TASK_LOOP: rd_task_lock()
	rf_handle_sleep_table(&rr_sleep_ttable, &rr_sleep_ttable_of);

#if D_ENABLE_MESSAGE_QUEUE
	rf_handle_msgq_table(rr_msgq_table);
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE
	rf_handle_mutex_table(rr_mutex_table);
#endif /* D_ENABLE_SEMAPHORE */

#if D_ENABLE_SOFT_TIMER
	rf_handle_timer_table(&rr_timer_table, &rr_timer_table_of);
#endif /* D_ENABLE_SOFT_TIMER */

	rd_enter_critical(csr)

    /* Must test tick overflow flag before check tick overflow */
    if (rd_test_tick_overflow()) {
        rd_reset_tick_overflow();
    }

    if (rr_tick > rr_task_tick) {
        rd_set_task_schedule();
    }

    if (rr_tick < rr_last_tick) {
        rd_set_tick_overflow();
    }

	/* Tell rf_schedule() that function is invoked by RTOS core */
	rd_set_rtos_invoke();

	/* Record the current rr_tick for checking tick overflow */
    rr_last_tick = rr_tick;

	rd_leave_critical(csr)

	/* RTOS core invoke the task scheduler to switch context */
    rd_task_unlock() rf_schedule(); goto _TASK_LOOP;
}

REGINA_PUBLIC void rf_setup_rtos(void)
{
    rf_setup_heap();

    rd_set_max_prior(D_MIN_TASK_PRIOR);
    rd_reset_rtos_running();
	rd_reset_task_schedule();
	rd_reset_tick_overflow();
    rd_reset_rtos_invoke();

    rr_tick = rr_last_tick = rr_task_lock = rr_task_tick = 0;
    rr_pt_task = rr_pc_task = rr_rd_task = rr_rt_task = NULL;

#if D_ENABLE_TASK_STAT
	rr_load_tick = 0;
#endif /* D_ENABLE_TASK_STAT */

    rr_rt_ttable = rf_create_tcb_table();
    rr_sleep_ttable_of = rf_create_tcb_table();
    rr_sleep_ttable = rf_create_tcb_table();
    rr_tick_task = rf_create_core_task(D_MAX_TASK_PRIOR, D_TICK_TASK_STACK_SIZE, rf_tick_task, NULL);
    rr_idle_task = rf_create_core_task(D_MIN_TASK_PRIOR, D_IDLE_TASK_STACK_SIZE, rf_idle_task, NULL);

#if D_ENABLE_SOFT_TIMER
	rr_timer_table_of = rf_create_timer_table();
	rr_timer_table = rf_create_timer_table();
#endif /* D_ENABLE_SOFT_TIMER */

#if D_ENABLE_MESSAGE_QUEUE
	rr_msgq_table = rf_create_msgq_table();
#endif /* D_ENABLE_MESSAGE_QUEUE */

#if D_ENABLE_SEMAPHORE
	rr_mutex_table = rf_create_mutex_table();
#endif /* D_ENABLE_SEMAPHORE */

    rf_setup_tick();
}

REGINA_PUBLIC void rf_start_rtos(void)
{
	/* Make sure run the user task with highest priority */
	rr_rd_task = rf_get_ready_task();
	rd_reset_task_schedule();

    rr_rt_task = rr_pt_task = rr_idle_task;
	rr_task_tick = rf_get_next_tick(D_TASK_TIME_SLICE);
	rf_start_core();
}

/********************************** End of The File ************************************/
