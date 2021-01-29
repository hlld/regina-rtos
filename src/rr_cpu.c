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

/**
 * Currently, Regina supports include STM32 F1/F4/F7 series. To fully 
 * support the Float Point Unit or FPU, some works have to be done. 
 * As We all know, STM32 F1 series do not have FPU, so we needn't to 
 * care about it. However, STM32 F4/F7 series have FPU. Thus we need 
 * to include header files form ST libraries to check the target CPU 
 * and to know what we need to do with its FPU.
 */
#if defined D_ARM_CORTEX_CM3
#ifdef USE_HAL_DRIVER
#include "stm32f1xx_hal.h"
#else
#include "stm32f10x.h"
#endif /* USE_HAL_DRIVER */
#elif defined D_ARM_CORTEX_CM4F
#ifdef USE_HAL_DRIVER
#include "stm32f4xx_hal.h"
#else
#include "stm32f4xx.h"
#endif /* USE_HAL_DRIVER */
#elif defined D_ARM_CORTEX_CM7
#ifdef USE_HAL_DRIVER
#include "stm32f7xx_hal.h"
#else
#include "stm32f7xx.h"
#endif /* USE_HAL_DRIVER */
#endif /* D_ARM_CORTEX_CM3 */

#if defined D_ARM_CORTEX_CM4F && __FPU_PRESENT && __FPU_USED
#define D_ARM_CORTEX_CM4F_USE_FPU
#elif defined D_ARM_CORTEX_CM7 && __FPU_PRESENT && __FPU_USED
#define D_ARM_CORTEX_CM7_USE_FPU
#endif /* D_ARM_CORTEX_CM4F && __FPU_PRESENT && __FPU_USED */

#if defined __clang__ || defined __GNUC__
#define D_ARM_GNUC_CLANG_COMPILE
#else
#define D_NOT_ARM_GNUC_CLANG_COMPILE
#endif /* __clang__ || __GNUC__ */

REGINA_IMPORT rt_uint rr_ctl;
REGINA_IMPORT rt_tcb* rr_rd_task;
REGINA_IMPORT rt_tcb* rr_rt_task;

REGINA_PRIVATE void rf_puts(const rt_char* pstr, rt_size xlen)
{
	rt_size it;

	rd_assert(pstr)

	for (it = 0; it < xlen; it++) {
    /* Write assert text through peripheral USART1 */
#if defined D_ARM_CORTEX_CM3 || defined D_ARM_CORTEX_CM4F
		while(!(USART1->SR & 0x40)) {
		}
		USART1->DR = pstr[it];
#elif defined D_ARM_CORTEX_CM7
		while(!(USART1->ISR & 0x40)){
		}
		USART1->TDR = pstr[it];
#endif /* D_ARM_CORTEX_CM3 || D_ARM_CORTEX_CM4F */
	}
}

REGINA_PRIVATE void rf_delay_us(rt_uint xusec)
{
    rt_uint ticks, told, reload, tnow, tcnt;
	rt_bool is_exit = rt_false;

	reload = SysTick->LOAD;
	told = SysTick->VAL;
	/* The SysTick clock must be HCLK */
	ticks = (SystemCoreClock / 1000000) * xusec;
	tcnt = 0;

	while (!is_exit) {
		tnow = SysTick->VAL;
		if(tnow != told) {
			if(tnow < told) {
				tcnt += told - tnow;
			}
			else {
				tcnt += reload - tnow + told;
			}
			told = tnow;
			if(tcnt >= ticks) {
				is_exit = rt_true;
			}
		}
	}
}

REGINA_PRIVATE void rf_delay_ms(rt_uint xmsec)
{
	while (xmsec) {
		rf_delay_us(1000);
		xmsec--;
	}
}

REGINA_PRIVATE rt_size* rf_setup_task_stack(
	rt_task_func pfunc, 
	rt_pvoid parg, 
	rt_size* stk_addr, 
	rt_task_exit pexit
) {
    rt_size* sp_addr;

#if defined D_ARM_CORTEX_CM3 || defined D_ARM_CORTEX_CM4F
	/* Align sp_addr to 4 bytes */
	sp_addr = (rt_size *)((rt_size)(stk_addr) & 0xFFFFFFFC);
#elif defined D_ARM_CORTEX_CM7
	/* Align sp_addr to 8 bytes */
	sp_addr = (rt_size *)((rt_size)(stk_addr) & 0xFFFFFFF8);
#endif /* D_ARM_CORTEX_CM3 || D_ARM_CORTEX_CM4F */

#if defined D_ARM_CORTEX_CM4F || defined D_ARM_CORTEX_CM7
    /* No name register */
    *(sp_addr) = (rt_size)0x00000000;
#endif /* D_ARM_CORTEX_CM4F || D_ARM_CORTEX_CM7 */

#if defined D_ARM_CORTEX_CM4F_USE_FPU
    /* FPSCR register */
	*(--sp_addr) = (rt_size)0x00001000;
    /* S15-S0 registers */
	sp_addr -= 16;
#endif /* D_ARM_CORTEX_CM4F_USE_FPU */

#if defined D_ARM_CORTEX_CM4F || defined D_ARM_CORTEX_CM7
    /* XPSR register */
	*(--sp_addr) = (rt_size)0x01000000;
#elif defined D_ARM_CORTEX_CM3
	/* XPSR register */
	*(sp_addr) = (rt_size)0x01000000;
#endif /* D_ARM_CORTEX_CM4F || D_ARM_CORTEX_CM7 */

    /* Entry point / PC register */
    *(--sp_addr) = (rt_size)pfunc;
    /* R14 / LR register */
    *(--sp_addr) = (rt_size)pexit;
    /* R1 - R3¡¢R12 registers */
	sp_addr -= 4;
    /* R0 (task argument) register */
    *(--sp_addr) = (rt_size)parg;

#if defined D_ARM_CORTEX_CM4F_USE_FPU
    /* S31-S16 registers */
	sp_addr -= 16;
#endif /* D_ARM_CORTEX_CM4F_USE_FPU */

    /* R4 - R11 registers */
	sp_addr -= 8;

#if defined D_ARM_CORTEX_CM7_USE_FPU
	/* FPSCR register */
	*(--sp_addr) = (rt_size)0x02000000;
	/* S31-S0 FPU registers */
	sp_addr -= 32;
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

	return sp_addr;
}

REGINA_PRIVATE void rf_setup_tick(void)
{
	/* Select HCLK as the SysTick clock */
	SysTick->CTRL |= (1u << 2);

	/* Interrupt per millisecond */
	SysTick->LOAD = SystemCoreClock / 1000;

	/* Enable SysTick interrupt */
	SysTick->CTRL |= (1u << 1);

	/* Enable SysTick */
	SysTick->CTRL |= (1u << 0);
}

#if defined D_NOT_ARM_GNUC_CLANG_COMPILE
/* REGINA_PRIVATE */ REGINA_ASSEM void rf_start_core(void)
{
	IMPORT rr_ctl

	PRESERVE8

    /* Set the priority of PendSV interrupt to lowest */
	ldr r0, =0xE000ED22
	ldr r1, =0xFF
	strb r1, [r0]

    /* Enable the Floating Point Unit support */
#if defined D_ARM_CORTEX_CM4F_USE_FPU || defined D_ARM_CORTEX_CM7_USE_FPU
    /* Load the address of CPACR register */
	ldr	r0, =0xE000ED88
	ldr	r1, [r0]
    /* Enable CP10 and CP11 coprocessors */
	orr	r1, r1, #(0xF <<20)
    str	r1, [r0]
#endif /* D_ARM_CORTEX_CM4F_USE_FPU || D_ARM_CORTEX_CM7_USE_FPU */

	/**
     * Disable automatically save FP registers
     * Disable the lazy context switch mode
	 */
#if defined D_ARM_CORTEX_CM7_USE_FPU
	/* Load the address of FPCCR register */
	ldr r0, =0xE000EF34
	ldr r1, [r0]
	/* Reset the LSPEN and ASPEN bit */
	ands r1, r1, #0x3FFFFFFF
	str r1, [r0]
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

    /* Reset the PSP register value */
	movs r0, #0x00
	msr psp, r0

    /* Set the D_RTOS_RUNNING flag */
	ldr r0, =rr_ctl
	ldr	r1, [r0]
	orrs r1, r1, #(D_RTOS_RUNNING_MASK)
	str	r1, [r0]

    /* Trigger PendSV interrupt */
	ldr	r0, =0xE000ED04
	ldr	r1, =0x10000000
	str	r1, [r0]

    /* Enable CPU interrupt */
	cpsie i
    /* Code stoped here and enter an infinite loop */
_BLOCK_LOOP
    b _BLOCK_LOOP

	ALIGN
}

/* REGINA_PRIVATE */ REGINA_ASSEM rt_size rf_enter_critical(void)
{
    PRESERVE8

	/* Store the primask and disable CPU interrupt */
	mrs r0, primask
	cpsid i
	bx lr

	ALIGN
}

/* REGINA_PRIVATE */ REGINA_ASSEM void rf_leave_critical(rt_size mask)
{
    PRESERVE8

    /* Restore the primask and enable CPU interrupt */
	msr primask, r0
	bx lr

	ALIGN
}

/* REGINA_PRIVATE */ REGINA_ASSEM void rf_switch_context(void)
{
    PRESERVE8

    /* Trigger PendSV interrupt */
	ldr r0, =0xE000ED04
	ldr r1, =0x10000000
	str r1, [r0]
	bx lr

	ALIGN
}

/* REGINA_PRIVATE */ REGINA_ASSEM void PendSV_Handler(void)
{
    IMPORT	rr_rt_task
	IMPORT	rr_rd_task

	PRESERVE8

    /* Disable CPU interrupt */
	cpsid i
    /* Skip save registers the first time */
	mrs	r0, psp
	cbz	r0, _handle_task

#if defined D_ARM_CORTEX_CM4F_USE_FPU
    /* Save S16-S31 registers if used FPU */
	tst	r14, #0x10
	it eq
	vstmdbeq r0!, {s16-s31}
#endif /* D_ARM_CORTEX_CM4F_USE_FPU */

    /* Save R4-R11 registers to rt_task stack */
	subs r0, r0, #0x20
	stm	r0, {r4-r11}

#if defined D_ARM_CORTEX_CM7_USE_FPU
    /* Save FPSCR and S0-S31 registers */
    vmrs r1, fpscr
    str r1, [r0, #-4]!
    vstmdb r0!, {s0-s31}
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

    /* rr_rt_task->stack_addr = R0 */
	ldr	r1, =rr_rt_task
	ldr	r1, [r1]
	str	r0, [r1]

_handle_task
    /* rr_rt_task = rr_rd_task */
	ldr	r0, =rr_rt_task
	ldr	r1, =rr_rd_task
	ldr	r2, [r1]
	str	r2, [r0]
	ldr	r0, [r2]

#if defined D_ARM_CORTEX_CM7_USE_FPU
    /* Restore FPSCR and S0-S31 registers */
	vldmia r0!, {s0-s31}
    ldmia r0!, {r1}
    vmsr fpscr, r1
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

    /* Restore R4-R11 from rr_rt_task stack */
	ldm	r0, {r4-r11}
	adds r0, r0, #0x20

#if defined D_ARM_CORTEX_CM4F_USE_FPU
    /* Restore S16-S31 registers */
	tst r14, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}
#endif /* D_ARM_CORTEX_CM4F_USE_FPU */

    /* PSP = rr_rt_task->stack_addr */
	msr	psp, r0
    /* Make sure use PSP register */
	orr	lr, lr, #0x04

    /* Enable CPU interrupt */
	cpsie i
	bx lr

	ALIGN
}
#endif /* D_NOT_ARM_GNUC_CLANG_COMPILE */

REGINA_PRIVATE void SysTick_Handler(void)
{
#ifdef USE_HAL_DRIVER
	HAL_IncTick();
#endif /* USE_HAL_DRIVER */

	rf_handle_tick_isr();
}

#if defined D_ARM_GNUC_CLANG_COMPILE
REGINA_PRIVATE void rf_start_core(void)
{
    __asm__ __volatile__ (
    ".align 8\n"
	".global rr_ctl\n"

    "ldr r0, =0xE000ED22\n"
    "ldr r1, =0xFF\n"
    "strb r1, [r0]\n"

#if defined D_ARM_CORTEX_CM4F_USE_FPU || defined D_ARM_CORTEX_CM7_USE_FPU
    "ldr r0, =0xE000ED88\n"
    "ldr r1, [r0]\n"
    "orr r1, r1, #(0xF <<20)\n"
    "str	r1, [r0]\n"
#endif /* D_ARM_CORTEX_CM4F_USE_FPU || D_ARM_CORTEX_CM7_USE_FPU */

#if defined D_ARM_CORTEX_CM7_USE_FPU
    "ldr r0, =0xE000EF34\n"
    "ldr r1, [r0]\n"
    "ands r1, r1, #0x3FFFFFFF\n"
    "str r1, [r0]\n"
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

	"movs r0, #0x00\n"
    "msr psp, r0\n"

    "ldr r0, =rr_ctl\n"
    "ldr r1, [r0]\n"
    "orrs r1, r1, %0\n"
    "str r1, [r0]\n"

    "ldr r0, =0xE000ED04\n"
    "ldr r1, =0x10000000\n"
    "str r1, [r0]\n"

    "cpsie i\n"
"_BLOCK_LOOP:\n"
    "b _BLOCK_LOOP\n"
    : 
	: "i" (D_RTOS_RUNNING_MASK)
	: "memory"
    );
}

REGINA_PRIVATE rt_size rf_enter_critical(void)
{
	rt_size result;

	__asm__ __volatile__ (
    	".align 8\n"
    	"mrs %0, primask\n"
    	"cpsid i\n"
    	"bx lr\n"
    	: "=r" (result)
		: 
		: "memory"
    );
}

REGINA_PRIVATE void rf_leave_critical(rt_size mask)
{
	__asm__ __volatile__ (
    	".align 8\n"
    	"msr primask, %0\n"
    	"bx lr\n"
    	: 
    	: "r" (mask)
		: "memory"
    );
}

REGINA_PRIVATE void rf_switch_context(void)
{
	__asm__ __volatile__ (
    	".align 8\n"
    	"ldr r0, =0xE000ED04\n"
    	"ldr r1, =0x10000000\n"
    	"str r1, [r0]\n"
    	"bx lr\n"
		: 
		: 
		: "memory"
    );
}

REGINA_PRIVATE void PendSV_Handler(void)
{
	__asm__ __volatile__ (
    	".align 8\n"
		".global rr_rt_task\n"
		".global rr_rd_task\n"

		"cpsid i\n"
		"mrs r0, psp\n"
		"cbz r0, _handle_task\n"

#if defined D_ARM_CORTEX_CM4F_USE_FPU
		"tst r14, #0x10\n"
		"it eq\n"
		"vstmdbeq r0!, {s16-s31}\n"
#endif /* D_ARM_CORTEX_CM4F_USE_FPU */

		"subs r0, r0, #0x20\n"
		"stm r0, {r4-r11}\n"

#if defined D_ARM_CORTEX_CM7_USE_FPU
		"vmrs r1, fpscr\n"
		"str r1, [r0, #-4]!\n"
		"vstmdb r0!, {s0-s31}\n"
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

		"ldr r1, =rr_rt_task\n"
		"ldr r1, [r1]\n"
		"str r0, [r1]\n"

"_handle_task:\n"
		"ldr r0, =rr_rt_task\n"
		"ldr r1, =rr_rd_task\n"
		"ldr r2, [r1]\n"
		"str r2, [r0]\n"
		"ldr r0, [r2]\n"

#if defined D_ARM_CORTEX_CM7_USE_FPU
		"vldmia r0!, {s0-s31}\n"
		"ldmia r0!, {r1}\n"
		"vmsr fpscr, r1\n"
#endif /* D_ARM_CORTEX_CM7_USE_FPU */

		"ldm r0, {r4-r11}\n"
		"adds r0, r0, #0x20\n"

#if defined D_ARM_CORTEX_CM4F_USE_FPU
		"tst r14, #0x10\n"
		"it eq\n"
		"vldmiaeq r0!, {s16-s31}\n"
#endif /* D_ARM_CORTEX_CM4F_USE_FPU */

		"msr psp, r0\n"
		"orr lr, lr, #0x04\n"

		"cpsie i\n"
		"bx lr\n"
    	: 
    	: 
		: "memory"
    );
}
#endif /* D_ARM_GNUC_CLANG_COMPILE */

/********************************** End of The File ************************************/
