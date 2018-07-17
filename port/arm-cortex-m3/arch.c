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


void sleep_us(INT32U x)
{
	DWORD ticks, told, reload, tnow, tcnt;
	
	D_tasks_lock()
	reload = D_tick_load;
	told = D_tick_val;
	ticks = (D_arch_clock / 1000000) * x;
	tcnt = 0;
	
	while(1) {
		tnow = D_tick_val;
		if(tnow != told) {
			if(tnow < told)
				tcnt += told - tnow;
			else
				tcnt += reload - tnow + told;
			told = tnow;
			if(tcnt >= ticks)
				break;
		}
	}
	D_tasks_unlock()
}

DWORD* task_stack_init(void (*pfunc)(void *arg), 
	void * uarg,
	DWORD* stack,
	void (*pexit)(void))
{
	DWORD *tspaddr = stack;

	tspaddr = (DWORD *)((DWORD)(tspaddr) & 0xFFFFFFFC);	
	*(tspaddr) = (DWORD)0x01000000;
    *(--tspaddr) = (DWORD)pfunc;
    *(--tspaddr) = (DWORD)pexit;

	tspaddr -= 4;
    *(--tspaddr) = (DWORD)uarg;
	tspaddr -= 8;
	return tspaddr;
}

void tick_init(void)
{
	D_tick_ctrl |= (DWORD)0x00000004;
	D_tick_ctrl |= 1ul << 1;
	D_tick_load  = ((D_arch_clock / 1000) * D_CORE_HEART_BEAT);
	D_tick_ctrl |= 1ul << 0;
}

__asm void start_core(void)
{
	import	g_ctrl
	preserve8
	
	ldr	r0, =0xE000ED22
	ldr	r1, =0xFF
	strb	r1, [r0]
	
	movs	r0, #0x00
	msr	psp, r0
	
	ldr	r0, =g_ctrl
	ldr	r1, [r0]
	orrs	r1, r1, #B_core_run
	str	r1, [r0]
	
	ldr	r0, =0xE000ED04
	ldr	r1, =0x10000000
	str	r1, [r0]
	cpsie	i
	b	.
	align
}

__asm DWORD enter_critical(void) 
{
	preserve8

	mrs	r0, primask
	cpsid	i
	bx	lr
	align
}

__asm void leave_critical(DWORD primask) 
{
	preserve8

	msr	primask, r0
	bx	lr
	align
}

__asm void task_switch(void)
{
	preserve8

	ldr	r0, =0xE000ED04
	ldr	r1, =0x10000000
	str	r1,[r0]
	bx	lr
	align
}

__asm void svc_handl(void) 
{
	import	g_rdtask
	import	g_rttask
	preserve8
	
	cpsid	i
	mrs	r0, psp
	cbz	r0, start_handl
	
	subs	r0, r0, #0x20
	stm	r0, {r4-r11}
	
	ldr	r1, =g_rttask
	ldr	r1, [r1]
	str	r0, [r1]
	
start_handl
	ldr	r0, =g_rttask
	ldr	r1, =g_rdtask
	ldr	r2, [r1]
	str	r2, [r0]
	
	ldr	r0, [r2]
	ldm	r0, {r4-r11}
	adds	r0, r0, #0x20
	
	msr	psp, r0
	orr	lr, lr, #0x04
	cpsie	i
	bx	lr
	align
}
/* ----------------------------------- End of file --------------------------------------- */
