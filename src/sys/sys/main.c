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
#include "flash.h"
#include "lcd.h"

int kputs(char *);

/*
 * initialisation function
 * We start here as soon as we have a stack.
 * Here we initialize the various systems
 * and devices that need to be initialized.
 */
STARTUP(void kmain())
{
	// TODO: move these to the appropriate headers
	void vtinit();
	void grayinit();
	void loadavinit();
	void battinit();
	void usageinit();
	void bogomips();

	calloutinit();
	lcdinit();
	meminit();
	grayinit();
	vtinit();
	linkinit();
	audioinit();
	sched_init();
	procinit();
	bufinit();
	flashinit();
	inodeinit();
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
	struct timespec realtime = getrealtime();
	if (realtime.tv_sec < 1000000000L) { /* before ~2001 */
		extern const unsigned long build_date;
		realtime.tv_sec = build_date;
		realtime.tv_nsec = 0;
		setrealtime(&realtime);
	}
	uptime.tv_sec = uptime.tv_nsec = 0;
	spl0();
	bogomips();
}
