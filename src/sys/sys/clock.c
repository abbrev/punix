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

void calloutinit()
{
	G.callout[0].c_func = NULL;
}

/*
 * This arranges for func(arg) to be called in time/HZ seconds.
 * The callout array is sorted in order of times as a delta list.
 * End of the callout array is marked with a sentinel entry (c_func == NULL).
 */
STARTUP(int timeout(void (*func)(void *), void *arg, long time))
{
	struct callout *c1, *c2;
	long t;
	int x;
	
	t = time;
	c1 = &G.callout[0];
	x = spl7();
	
	//kprintf("timeout: adding a timeout in %ld ticks\n", time);
	while (c1->c_func != NULL && c1->c_dtime <= t) {
		t -= c1->c_dtime;
		++c1;
	}
	
	c2 = c1;
	
	/* find the sentinel (NULL entry) */
	while (c2->c_func != NULL)
		++c2;
	
	/* any room to put this new entry? */
	if (c2 >= &G.callout[NCALL-1])
		return -1;
	
	if (c1->c_func)
		c1->c_dtime -= t;
	
	/* move entries upward to make room for this new entry */
	do {
		c2[1] = c2[0];
		--c2;
	} while (c2 >= c1);
	
	c1->c_dtime = t;
	c1->c_func = func;
	c1->c_arg = arg;
	
	splx(x);
	return 0;
}

/*
 * Remove a pending callout.
 * Return zero if no timeout was removed, or non-zero if one was removed.
 */
STARTUP(int untimeout(void (*func)(void *), void *arg))
{
	struct callout *cp;
	int x;
	int canhastimeout = 0;
	x = spl7();
	for (cp = &G.callout[0]; cp < &G.callout[NCALL]; ++cp) {
		if (cp->c_func == func && cp->c_arg == arg) {
			canhastimeout = 1;
			if (cp < &G.callout[NCALL-1] && cp[1].c_func)
				cp[1].c_dtime += cp[0].c_dtime;
			while (cp < &G.callout[NCALL-1]) {
				*cp = *(cp+1);
				++cp;
			}
			G.callout[NCALL-1].c_func = NULL;
			break; /* remove only the first timeout */
		}
	}
	splx(x);
	return canhastimeout;
}

/* schedule a task until after all interrupts are handled */
STARTUP(int defer(void (*func)(void *), void *arg))
{
	return timeout(func, arg, 0);
}

/* unschedule a deferred task */
STARTUP(int undefer(void (*func)(void *), void *arg))
{
	return untimeout(func, arg);
}
