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

#ifndef __ARCH_H__
#define __ARCH_H__


#include <stdio.h>
#include "config.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned char 					INT8U;
typedef unsigned short 					INT16U;
typedef unsigned int 					INT32U;
typedef float 							FLOAT;
typedef unsigned int 					RESULT;
typedef unsigned int 					DWORD;


#ifndef NULL
#define	NULL							0
#endif
#ifndef TRUE
#define	TRUE							1
#endif
#ifndef FALSE
#define	FALSE							0
#endif


#define D_halt_tick				0xFFFFFFFF
#define D_task_lab				0xAAAAAAAA
#define D_heap_lab				0x80000000


#define D_arch_clock			72000000u
#define D_tick_vala				0xE000E018
#define D_tick_val				(*((DWORD *) (D_tick_vala)))
#define D_tick_loada			0xE000E014
#define D_tick_load				(*((DWORD *) (D_tick_loada)))
#define D_tick_ctrla			0xE000E010
#define D_tick_ctrl				(*((DWORD *) (D_tick_ctrla)))


#define D_enter_critical(x)		{(x) = enter_critical();}
#define D_leave_critical(x)		{leave_critical((x));}


#define svc_handl 				PendSV_Handler
#define tick_handl				SysTick_Handler
#define D_print					printf


void sleep_us(INT32U x);
DWORD* task_stack_init(void (*pfunc)(void *uarg), void * uarg, DWORD* stkaddr, void (*pexit)(void));
void tick_init(void);
void start_core(void);
DWORD enter_critical(void);
void leave_critical(DWORD primask);
void task_switch(void);
#ifdef __cplusplus
	}
#endif
#endif
/* ----------------------------------- End of file --------------------------------------- */
