/*
 * Punix, Puny Unix kernel
 * Copyright 2008 Christopher Williams
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

#include <limits.h>

#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "callout.h"
#include "globals.h"

STARTUP(long hzto(struct timespec *tv))
{
	long ticks, sec;
	struct timespec diff;
	int x = spl7();
	
	timespecsub(&realtime, tv, &diff);
	
	if ((diff.tv_sec + 1) <= LONG_MAX / HZ)
		ticks = diff.tv_sec * HZ + diff.tv_nsec / TICK;
	else
		ticks = LONG_MAX;
	
	splx(x);
	return ticks;
}

/*
 * This arranges for func(arg) to be called in time/HZ seconds.
 * The callout array is sorted in order of times as a delta list.
 */
STARTUP(int timeout(void (*func)(void *), void *arg, long time))
{
	struct callout *c1, *c2;
	long t;
	int x;
	
	t = time;
	c1 = &G.callout[0];
	x = spl7();
	
	while (c1->c_func != NULL && c1->c_time <= t) {
		t -= c1->c_time;
		++c1;
	}
	
	if (c1 >= &G.callout[NCALL-1])
		return -1;
	
	c1->c_time -= t;
	c2 = c1;
	
	/* find the last callout entry */
	while (c2->c_func != NULL)
		++c2;
	c2[1].c_func = NULL;
	
	/* move entries upward to make room for this new entry */
	while (c2 >= c1) {
		c2[1] = c2[0];
		--c2;
	}
	
	c1->c_time = t;
	c1->c_func = func;
	c1->c_arg = arg;
	
	splx(x);
	return 0;
}

/*
 * remove a pending callout
 */
STARTUP(void untimeout(void (*func)(void *), void *arg))
{
	struct callout *cp;
	int x;
	x = spl7();
	for (cp = &G.callout[0]; cp < &G.callout[NCALL]; ++cp) {
		if (cp->c_func == func && cp->c_arg == arg) {
			do
				*cp = *(cp+1);
			while (cp->c_func);
			break;
		}
	}
	splx(x);
}
