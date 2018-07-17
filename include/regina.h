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

#ifndef __REGINA_H__
#define __REGINA_H__


#include "arch.h"


#ifdef __cplusplus
extern "C" {
#endif
#define D_kp_wait			D_halt_tick
#define D_no_wait			0u
#if D_ENABLE_STACK_CHECK
#define D_stk_offset		10u
#endif
#define	D_max_tprior		0u
#define	D_min_tprior		0xFFFF


#define	V_tcb_mn			0xA1A1
#define	V_tim_mn			0xA2A2
#define	V_msg_mn			0xA4A4
#define	V_sem_mn			0xA8A8

#define B_rtos_mn			0x0000FFFF
#define B_core_maxp			0x0000FFFF
#define B_core_run			0x00010000
#define B_core_tsd			0x00020000
#define B_core_isof			0x00040000

#define N_rtos_mn			0u
#define N_core_maxp			0u
#define N_core_run			16u
#define N_core_tsd			17u
#define N_core_isof			18u


#define D_test_bit(f, b)	((f) & (b))
#define D_get_magic(f)		(((f) & B_rtos_mn) >> N_rtos_mn)
#define D_reset_magic(f)	(f) &= ~B_rtos_mn
#define D_set_magic(f, v)		\
{								\
	(f) &= ~B_rtos_mn;			\
	(f) |= ((v) & B_rtos_mn);	\
}

#define D_get_maxp()	((g_ctrl & B_core_maxp) >> N_core_maxp)
#define D_reset_maxp()	g_ctrl &= ~B_core_maxp;
#define D_set_maxp(v)				\
{									\
	g_ctrl &= ~B_core_maxp;			\
	g_ctrl |= ((v) & B_core_maxp);	\
}

#define D_test_run()	D_test_bit(g_ctrl, B_core_run)
#define D_reset_run()	g_ctrl &= ~B_core_run
#define D_set_run()		g_ctrl |= B_core_run

#define D_test_tsd()	D_test_bit(g_ctrl, B_core_tsd)
#define D_reset_tsd()	g_ctrl &= ~B_core_tsd;
#define D_set_tsd()		g_ctrl |= B_core_tsd;

#define D_test_isof()	D_test_bit(g_ctrl, B_core_isof)
#define D_reset_isof()	g_ctrl &= ~B_core_isof
#define D_set_isof()	g_ctrl |= B_core_isof


#define	D_tasks_lock() 	g_slock++;
#define	D_tasks_unlock()	\
{							\
	D_assert(g_slock)		\
	g_slock--;				\
}


#if D_ENABLE_ASSERT_OUTPUT
#define D_assert(x) 									\
{														\
	if (!(x))											\
		D_print("Error: %s, %d\n", __FILE__, __LINE__);	\
}
#else
#define D_assert(x)
#endif


#if D_ENABLE_SOFT_TIMER
#define	V_tim_nof			1u
#define	V_tim_of			2u

#define B_tim_isp			0x00010000
#define B_tim_isof			0x00060000

#define N_tim_isp			16u
#define N_tim_isof			17u

#define D_get_timisof(f)	(((f) & B_tim_isof) >> N_tim_isof)
#define D_reset_timisof(f)	(f) &= ~B_tim_isof
#define D_set_timisof(f, v)						\
{												\
	(f) &= ~B_tim_isof;							\
	(f) |= (((v) << N_tim_isof) & B_tim_isof);	\
}

#define D_test_timisp(f)	D_test_bit((f), B_tim_isp)
#define D_reset_timisp(f)	(f) &= ~B_tim_isp
#define D_set_timisp(f)		(f) |= B_tim_isp

typedef void*	T_tim_handl;
typedef struct timer
{
	INT32U	flags;
	void*	pfunc;
	void*	uarg;
	DWORD	period;
	DWORD 	atick;
	
	struct timer*	next;
	struct timer* 	last;
} T_tim;

typedef struct ttab
{
	INT16U	total;
	
	T_tim*	head;
} T_ttab;
#endif


#define V_tcb_rd			1u
#define V_tcb_ws			2u
#define V_tcb_wsof			3u

#define B_tcb_ts			0x00030000
#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
#define B_tcb_wr			0x00040000
#endif
#if D_ENABLE_MESSAGE_QUEUE
#define B_tcb_isf			0x00080000
#endif
#if D_ENABLE_SEMAPHORE
#define B_tcb_wbad			0x00100000
#endif

#define N_tcb_ts			16u
#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
#define N_tcb_wr			18u
#endif
#if D_ENABLE_MESSAGE_QUEUE
#define N_tcb_isf			19u
#endif
#if D_ENABLE_SEMAPHORE
#define N_tcb_wbad			20u
#endif

#define D_get_tcbts(f)		(((f) & B_tcb_ts) >> N_tcb_ts)
#define D_reset_tcbts(f)	(f) &= ~B_tcb_ts
#define D_set_tcbts(f, v)					\
{											\
	(f) &= ~B_tcb_ts;						\
	(f) |= (((v) << N_tcb_ts) & B_tcb_ts);	\
}

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
#define D_test_tcbwr(f)		D_test_bit((f), B_tcb_wr)
#define D_reset_tcbwr(f)	(f) &= ~B_tcb_wr
#define D_set_tcbwr(f)		(f) |= B_tcb_wr
#endif

#if D_ENABLE_SEMAPHORE
#define D_test_tcbwbad(f)	D_test_bit((f), B_tcb_wbad)
#define D_reset_tcbwbad(f)	(f) &= ~B_tcb_wbad
#define D_set_tcbwbad(f)	(f) |= B_tcb_wbad
#endif

#if D_ENABLE_MESSAGE_QUEUE
#define D_test_tcbisf(f)	D_test_bit((f), B_tcb_isf)
#define D_reset_tcbisf(f)	(f) &= ~B_tcb_isf
#define D_set_tcbisf(f)		(f) |= B_tcb_isf
#endif

typedef void*	T_task_handl;
typedef struct tcb 
{
	DWORD*	tspaddr;
	
	INT32U	flags;
	INT16U	prior;
	DWORD	wkptick;
	DWORD*	stkaddr;
	
#if D_ENABLE_STACK_CHECK
	DWORD*	topstk;
#endif

#if D_ENABLE_TASK_STAT
	DWORD	rtick;
	FLOAT	rate;
#endif

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
	void*	lhead;
#endif

#if D_ENABLE_MESSAGE_QUEUE
	void*	pdata;
	DWORD	size;
#endif

#if D_ENABLE_SEMAPHORE
	INT16U	wprior;
	INT32U	windex;
	void**	wlist;
#endif

	struct tcb*	next;
	struct tcb*	last;
} T_tcb;

#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
typedef struct list 
{
	T_tcb*	ptcb;
	
	struct list*	next;
	struct list*	last;
} T_list;
#endif

typedef struct tab 
{
	INT16U	total;
	T_tcb*	lpre;
	T_tcb*	head;
} T_tab;


#if D_ENABLE_MESSAGE_QUEUE
#define B_msg_lock			0x00010000
#define B_msg_isqf			0x00020000
#define B_msg_isqe			0x00040000
#define B_msg_isrxe			0x00080000
#define B_msg_istxe			0x00100000

#define N_msg_lock			16u
#define N_msg_isqf			17u
#define N_msg_isqe			18u
#define N_msg_isrxe			19u
#define N_msg_istxe			20u

#define D_test_msglock(f)	D_test_bit((f), B_msg_lock)
#define D_reset_msglock(f)	(f) &= ~B_msg_lock
#define D_set_msglock(f)	(f) |= B_msg_lock

#define D_test_msgisqf(f)	D_test_bit((f), B_msg_isqf)
#define D_reset_msgisqf(f)	(f) &= ~B_msg_isqf
#define D_set_msgisqf(f)	(f) |= B_msg_isqf

#define D_test_msgisqe(f)	D_test_bit((f), B_msg_isqe)
#define D_reset_msgisqe(f)	(f) &= ~B_msg_isqe
#define D_set_msgisqe(f)	(f) |= B_msg_isqe

#define D_test_msgisrxe(f)	D_test_bit((f), B_msg_isrxe)
#define D_reset_msgisrxe(f)	(f) &= ~B_msg_isrxe
#define D_set_msgisrxe(f)	(f) |= B_msg_isrxe

#define D_test_msgistxe(f)	D_test_bit((f), B_msg_istxe)
#define D_reset_msgistxe(f)	(f) &= ~B_msg_istxe
#define D_set_msgistxe(f)	(f) |= B_msg_istxe

typedef void*	T_msgq_handl;
typedef struct msg
{
	void	*pdata;
	DWORD	size;
} T_msg;

typedef struct q {
	INT16U	front;
	INT16U	rear;
	T_msg	data[D_MESSAGE_QUEUE_LENGTH];
} T_queue;

typedef struct msgq
{
	INT32U	flags;
	T_queue*	queue;
	T_list*	wrxh;
	T_list*	wtxh;
} T_msgq;
#endif


#if D_ENABLE_SEMAPHORE
#define V_sem_binary		1u
#define V_sem_count			2u
#define V_sem_mutex			3u
#define V_sem_remutex		4u

#define B_sem_lock			0x00010000
#define B_sem_type			0x000E0000
#define B_sem_isrxe			0x00100000

#define N_sem_lock			16u
#define N_sem_type			17u
#define N_sem_isrxe			20u

#define D_test_bno(f, n)	((f) & (1 << (n)))
#define D_reset_bno(t, bit) (t) &= ~(1 << (bit))

#define D_get_semtype(f)	(((f) & B_sem_type) >> N_sem_type)
#define D_reset_semtype(f)	(f) &= ~B_sem_type
#define D_set_semtype(f, v)						\
{												\
	(f) &= ~B_sem_type;							\
	(f) |= (((v) << N_sem_type) & B_sem_type);	\
}

#define D_test_semlock(f)	D_test_bit((f), B_sem_lock)
#define D_reset_semlock(f)	(f) &= ~B_sem_lock
#define D_set_semlock(f)	(f) |= B_sem_lock

#define D_test_semisrxe(f)	D_test_bit((f), B_sem_isrxe)
#define D_reset_semisrxe(f)	(f) &= ~B_sem_isrxe
#define D_set_semisrxe(f)	(f) |= B_sem_isrxe

#define D_index_init(t, nbit)	\
{								\
	(t) = 0xffffffff;			\
	(t) <<= (nbit);				\
	(t) = ~(t);					\
}

typedef void*	T_sem_handl;
typedef struct sem 
{
	INT32U	flags;
	DWORD	value;
	T_tcb*	holder;
	T_list*	wrxh;
} T_sem;
#endif


extern DWORD	g_ctrl;
extern DWORD 	g_slock;
extern DWORD 	g_btick;
extern DWORD 	g_ltick;
extern DWORD 	g_stick;
#if D_ENABLE_TASK_STAT
extern DWORD	g_ltmark;
#endif
extern T_tcb* 	g_IDLE;
extern T_tcb* 	g_TICK;
extern T_tcb* 	g_temp;
extern T_tcb* 	g_rdtask;
extern T_tcb* 	g_rttask;
extern T_tab* 	g_rdtab;
extern T_tab* 	g_wstab[2];
#if D_ENABLE_SOFT_TIMER
extern T_ttab*	g_ttab[2];
#endif


void task_scheduler(INT8U isnext, INT8U isexit);
void heap_init(void);
T_tcb* create_core_task(INT16U prior, INT16U stksize, void (*pfunc)(void *uarg));
T_tab* create_task_table(void);
T_tcb* get_ready_task(void);
T_tab* get_task_table(T_tcb* tcb);
void move_task_to_table(T_tab *tab, T_tcb *tcb);
void remove_task_from_table(T_tab *tab, T_tcb *tcb);
void clean_overflowed_task_table(void);
DWORD get_next_tick(DWORD tick);
#if D_ENABLE_SOFT_TIMER
T_ttab* create_timer_table(void);
void timer_handl(void);
#endif
#if D_ENABLE_MESSAGE_QUEUE
T_queue* create_msg_queue(void);
RESULT remove_element_from_queue(T_queue* q, T_msg* pos);
RESULT move_element_to_queue(T_queue* q, T_msg* pos);
RESULT read_first_element(T_queue* q, T_msg* pos);
RESULT read_last_element(T_queue* q, T_msg* pos);
RESULT test_queue_full(T_queue* q);
RESULT test_queue_empty(T_queue* q);
#endif
#if D_ENABLE_MESSAGE_QUEUE || D_ENABLE_SEMAPHORE
T_list* create_list_node(T_tcb* tcb);
void move_node_to_list(T_list *head, T_list *list);
T_list* get_list_node(T_list *head, T_tcb *tcb);
void remove_node_from_list(T_list *list);
void clean_list_node(T_tcb* tcb);
RESULT test_list_empty(T_list *head);
#endif
#if D_ENABLE_STACK_CHECK
void stack_overflow_check(T_tcb* tcb);
#endif
#if D_ENABLE_TASK_STAT
void task_stat(T_tcb* tcb);
#endif
#if D_ENABLE_HEART_BEAT_HOOK
void tick_irq_hook(void);
#endif
#if D_ENABLE_SWITCH_HOOK
void task_switch_in_hook(T_task_handl handl);
void task_switch_out_hook(T_task_handl handl);
#endif
#if D_ENABLE_IDLE_TASK_HOOK
void IDLE_task_hook(void);
#endif


DWORD I_get_free_heap_lrecord(void);
DWORD I_get_free_heap_size(void);
DWORD I_get_rttick(void);
DWORD I_get_rttaskid(void);
void I_rtos_init(void);
void I_sleep_ms(FLOAT xms);
void* I_allocate(DWORD size);
void I_release(void* ptr);
void I_rtos_start(void);
void I_create_task(T_task_handl* handl, INT16U prior, INT16U stksize, void (*pfunc)(void *uarg), void* uarg);
void I_drop_task(T_task_handl *handl);
void I_suspend_task(T_task_handl handl);
void I_restore_task(T_task_handl handl);
void I_task_sleep(DWORD xms);
void I_task_exit(void);
#if D_ENABLE_TASK_STAT
DWORD I_get_IDLE_rtick(void);
FLOAT I_get_IDLE_rrate(void);
DWORD I_get_rttask_rtick(void);
FLOAT I_get_rttask_rrate(void);
#endif
#if D_ENABLE_SOFT_TIMER
void I_create_timer(T_tim_handl *handl, INT8U isperiodic, DWORD period, void (*pfunc)(void *uarg), void *uarg);
void I_drop_timer(T_tim_handl *handl);
void I_start_timer(T_tim_handl handl);
void I_stop_timer(T_tim_handl handl);
void I_set_timer_arg(T_tim_handl handl, void * uarg);
#endif
#if D_ENABLE_MESSAGE_QUEUE
void I_create_msgq(T_msgq_handl *handl);
void I_drop_msgq(T_msgq_handl *handl);
void I_lock_msgq(T_msgq_handl handl);
void I_unlock_msgq(T_msgq_handl handl);
RESULT I_send_msg(T_msgq_handl handl, void *pdata, DWORD size, DWORD afterms);
RESULT I_send_msgisr(T_msgq_handl handl, void *pdata, DWORD size);
RESULT I_wait_for_msg(T_msgq_handl handl, INT8U isfetch, DWORD afterms);
void* I_get_msg_data(void);
DWORD I_get_msg_size(void);
#endif
#if D_ENABLE_SEMAPHORE
void I_create_sem(T_sem_handl* handl, INT8U type, DWORD initv);
void I_drop_sem(T_sem_handl *handl);
void I_lock_sem(T_sem_handl handl);
void I_unlock_sem(T_sem_handl handl);
RESULT I_post_sem(T_sem_handl handl);
RESULT I_post_semisr(T_sem_handl handl);
RESULT I_wait_for_sem(T_sem_handl handl, DWORD afterms);
RESULT I_wait_for_mults(T_sem_handl* handl, DWORD afterms);
#endif
#ifdef __cplusplus
	}
#endif
#endif
/* ----------------------------------- End of file --------------------------------------- */
