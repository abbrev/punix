/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Chris Williams
 * 
 * $Id: main.c,v 1.9 2008/01/25 19:44:57 fredfoobar Exp $
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

#include "cell.h"

/*
 * initialisation function
 * we start here as soon as we have a stack
 * here we
 * 	setup core
 * 	setup root filesystem
 * 	build process 0
 */
STARTUP(void kmain())
{
	struct proc **pp;
	int i;
	short *horline;
	
	lcdinit();
#if 1
	vtinit();
#endif
	meminit();
	linkinit();
	audioinit();
	
	for (horline = (short *)&LCD_MEM[LCD_INCY*6*NUMCELLROWS];
	 horline < (short *)&LCD_MEM[LCD_INCY*6*NUMCELLROWS+LCD_INCY];
	  ++horline)
		*horline = 0xffff;
	
#if 1
	kputs(OS_NAME " build " BUILD "\n");
#else
	kputs(OS_NAME " v" OS_VERSION "\n");
#endif
	kputs(
	 "Copyright 2005-2008 Christopher Williams <abbrev@gmail.com>\n"
	 "Some portions copyright 2003, 2005 PpHd\n"
	 "\n"
	 "This program comes with ABSOLUTELY NO WARRANTY.\n"
	 "You may redistribute copies of this program\n"
	 "under the terms of the GNU General Public License.\n"
	 "\n");
	
	/* initialize core (setup heap and memory resources) */
	
	/* initialize proc structure */
	/* are these values correct and all I need? */
	for (pp = &G.proc[0]; pp < &G.proc[NPROC]; ++pp)
		*pp = 0;
	
	for (i = 0; i < NOFILE; ++i)
		P.p_ofile[i] = NULL;
	
	CURRENT = palloc();
	/* assert(CURRENT); */
	
	/* construct our first proc (awww, how cute!) */
	P.p_pid = 1;
	P.p_uid = P.p_ruid = P.p_svuid = 0;
	P.p_gid = P.p_rgid = P.p_svgid = 0;
	P.p_status = P_RUNNING;
	P.p_cputime = 0;
	P.p_nice = NZERO;
	P.p_basepri = PUSER;
	
	/* FIXME: move this to an appropriate place */
	for (i = 0; i < 7; ++i)
		P.p_rlimit[i].rlim_cur = P.p_rlimit[i].rlim_max = RLIM_INFINITY;
	
	setpri(&P);
	cputime = 0;
	setrun(&P);
}
