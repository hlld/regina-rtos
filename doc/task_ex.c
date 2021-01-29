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

#define task_prior 1u
#define task_stack_size 200u
rt_task_handle task_hdl;

void taks_func(rt_pvoid parg)
{
	for (; ;) {
		rf_sleep(500);
		rf_print("Regina task is running\n");
	}
}

int main(int argc, char* argv[])
{
	rf_setup_rtos();

	rf_create_task(task_hdl, task_prior, task_stack_size, taks_func, NULL);

	rf_start_rtos();
}

