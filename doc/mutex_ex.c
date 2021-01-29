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

#define task1_prior 1u
#define task1_stack_size 200u
rt_task_handle task1_hdl;

#define task2_prior 2u
#define task2_stack_size 200u
rt_task_handle task2_hdl;

#define task3_prior 3u
#define task3_stack_size 200u
rt_task_handle task3_hdl;

rt_mutex_handle mutex_hdl;
rt_lock_handle lock_hdl;

void taks1_func(rt_pvoid parg)
{
	for (; ;) {
		rf_request_mutex(mutex_hdl, 1000);
		/* The user code here */
		rf_release_mutex(mutex_hdl);

		rf_release_lock(lock_hdl);

		rf_sleep(500);
	}
}

void taks2_func(rt_pvoid parg)
{
	rt_result result;

	for (; ;) {
		result = rf_request_mutex(mutex_hdl, 1000);
		/* result = rf_request_mutex(mutex_hdl, D_BLOCK_TILL_DONE); */
		/* result = rf_request_mutex(mutex_hdl, D_RET_ONCE_ASK); */
		if (result) {
			rf_print("Mutex Requested\n");
		}
		rf_release_mutex(mutex_hdl);
	}
}

void taks3_func(rt_pvoid parg)
{
	rt_result result;

	for (; ;) {
		result = rf_request_lock(lock_hdl, 1000);
		/* result = rf_request_lock(lock_hdl, D_BLOCK_TILL_DONE); */
		/* result = rf_request_lock(lock_hdl, D_RET_ONCE_ASK); */
		if (result) {
			rf_print("Lock Requested\n");
		}
	}
}

int main(int argc, char* argv[])
{
	rf_setup_rtos();

	rf_create_task(task1_hdl, task1_prior, task1_stack_size, taks1_func, NULL);
	rf_create_task(task2_hdl, task2_prior, task2_stack_size, taks2_func, NULL);
	rf_create_task(task3_hdl, task3_prior, task3_stack_size, taks3_func, NULL);

	rf_create_mutex(mutex_hdl);
	rf_create_lock(lock_hdl);

	rf_start_rtos();
}

