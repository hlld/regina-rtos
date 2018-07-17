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


#if D_ENABLE_MESSAGE_QUEUE
#define D_copy_element(t, s)						\
{													\
	(t)->pdata = (s)->pdata;						\
	(t)->size = (s)->size;							\
}

RESULT test_queue_full(T_queue* q)
{
	if (((q->rear + 1) % D_MESSAGE_QUEUE_LENGTH) == q->front)
		return TRUE;
	return FALSE;
}

RESULT test_queue_empty(T_queue* q)
{
	if (q->front == q->rear)
		return TRUE;
	return FALSE;
}

RESULT move_element_to_queue(T_queue* q, T_msg* pos)
{
	if (((q->rear + 1) % D_MESSAGE_QUEUE_LENGTH) == q->front)
		return FALSE;
	else {
		q->rear = (q->rear + 1) % D_MESSAGE_QUEUE_LENGTH;
		D_copy_element(&(q->data[q->rear]), pos);
	}
	return TRUE;
}

RESULT remove_element_from_queue(T_queue* q, T_msg* pos)
{
	if (q->front == q->rear)
		return FALSE;
	else {
		q->front = (q->front + 1) % D_MESSAGE_QUEUE_LENGTH;
		D_copy_element(pos, &(q->data[q->front]));
	}
	return TRUE;
}

RESULT read_first_element(T_queue* q, T_msg* pos)
{
	if (q->front == q->rear)
		return FALSE;
	else
		D_copy_element(pos, &(q->data[(q->front + 1) % D_MESSAGE_QUEUE_LENGTH]));
	return TRUE;
}

RESULT read_last_element(T_queue* q, T_msg* pos)
{
	if (q->front == q->rear)
		return FALSE;
	else
		D_copy_element(pos, &(q->data[q->rear]));
	return TRUE;
}

T_queue* create_msg_queue(void)
{
	INT16U index;
	T_queue* q;
	
	q = (T_queue *)I_allocate(sizeof(T_queue));
	D_assert(q)
	
	q->rear = q->front = 0;
	for (index = 0; index < D_MESSAGE_QUEUE_LENGTH; index++) {
		q->data[index].pdata = NULL;
		q->data[index].size = 0;
	}
	return q;
}
#endif
/* ----------------------------------- End of file --------------------------------------- */
