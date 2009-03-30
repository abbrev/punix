/*
 * Punix, Puny Unix kernel
 * Copyright 2007, 2008 Christopher Williams
 * 
 * $Id$
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This implements a character FIFO queue using a circular buffer.
 */

#include "punix.h"
#include "param.h"
#include "queue.h"
#include "proc.h"
#include "inode.h"
#include "globals.h"

#if 0
STARTUP(int qisfull(struct queue *qp))
{
	return (qp->q_count >= QSIZE);
}

STARTUP(int qisempty(struct queue *qp))
{
	return (qp->q_count == 0);
}
#endif

STARTUP(void qclear(struct queue *qp))
{
	int x = spl5();
	
	qp->q_count = 0;
	qp->q_head = qp->q_tail = &qp->q_buf[0];
	
	splx(x);
}

STARTUP(int putc(int ch, struct queue *qp))
{
	int x = spl5(); /* higher than character devices */
	
	if (qp->q_count >= QSIZE) {
		ch = -1;
		goto out;
	}
	
	*qp->q_head++ = ch;
	if (qp->q_head >= &qp->q_buf[QSIZE])
		qp->q_head -= QSIZE;
	
	++qp->q_count;
	ch = 0;
	
out:
	splx(x);
	return ch;
}

STARTUP(int getc(struct queue *qp))
{
	int ch;
	int x = spl5();
	
	if (qp->q_count == 0) {
		ch = -1;
		goto out;
	}
	
	ch = (unsigned char)*qp->q_tail++;
	if (qp->q_tail >= &qp->q_buf[QSIZE])
		qp->q_tail -= QSIZE;
	
	--qp->q_count;
	
out:
	splx(x);
	return ch;
}
