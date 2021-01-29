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

rt_msgq_handle msgq_hdl;

void taks1_func(rt_pvoid parg)
{
	rt_msg msg = {
		.size = 0,
		.pdata = NULL
	};

	for (; ;) {
		rf_sleep(500);
		msg.size++;
		rf_send_message(msgq_hdl, &msg, 1000);
		/* rf_send_message(msgq_hdl, &msg, D_BLOCK_TILL_DONE); */
		/* rf_send_message(msgq_hdl, &msg, D_RET_ONCE_ASK); */
	}
}

void taks2_func(rt_pvoid parg)
{
	rt_result result;
	rt_msg msg;

	for (; ;) {
		result = rf_receive_message(msgq_hdl, &msg, 1000);
		/* result = rf_receive_message(msgq_hdl, &msg, D_BLOCK_TILL_DONE); */
		/* result = rf_receive_message(msgq_hdl, &msg, D_RET_ONCE_ASK); */
		if (result) {
			rf_print("Received Message: .size %d %X\n", msg.size);
		}
	}
}

int main(int argc, char* argv[])
{
	rf_setup_rtos();

	rf_create_task(task1_hdl, task1_prior, task1_stack_size, taks1_func, NULL);
	rf_create_task(task2_hdl, task2_prior, task2_stack_size, taks2_func, NULL);

	rf_create_msgq(msgq_hdl);

	rf_start_rtos();
}

