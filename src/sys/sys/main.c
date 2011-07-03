/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
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

#include <assert.h>
#include <stddef.h>

#include "punix.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"
#include "process.h"
#include "link.h"
#include "audio.h"

int kputs(char *);

/*
 * initialisation function
 * We start here as soon as we have a stack.
 * Here we initialize the various systems
 * and devices that need to be initialized.
 */
STARTUP(void kmain())
{
	calloutinit();
	lcdinit();
	vtinit();
	meminit();
	linkinit();
	audioinit();
	sched_init();
	procinit();
	bufinit();
	flashinit();
	loadavinit();
	battinit();
	usageinit();
	
#if 1
	kprintf("%s build %s\n", uname_sysname, uname_version);
#else
	kprintf("%s v%s\n", uname_sysname, uname_release);
	kputs(
	 "Copyright 2005-2011 Christopher Williams <abbrev@gmail.com>\n"
	 "Some portions copyright 2003, 2005 PpHd\n"
	 "\n"
	 "This program comes with ABSOLUTELY NO WARRANTY.\n"
	 "You may redistribute copies of this program\n"
	 "under the terms of the GNU General Public License.\n"
	 "\n");
#endif
	if (realtime.tv_sec < 1000000000L) { /* before ~2001 */
		realtime.tv_sec = REALTIME;
		realtime.tv_nsec = 0;
	}
	G.seconds = realtime.tv_sec;
	uptime.tv_sec = uptime.tv_nsec = 0;
	spl0();
	bogomips();
	kputchar('\n');
}
