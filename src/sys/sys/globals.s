/*
 * globals.s
 * Copyright 2008 Christopher Williams
 * 
 * $Id: globals.s,v 1.2 2008/01/16 07:02:25 fredfoobar Exp $
 * 
 * These are variables that may be accessed in either C or assembly routines.
 */

.global realtime, runrun, ioport, cputime, updlock, exec_ram
.global G

.section notabsolute,"w"
|.org 0x5c00

.if 0
realtime:	.skip	4
runrun:	.skip	2
ioport:	.skip	2
cputime:	.skip	2
updlock:	.skip	2
.endif
exec_ram:	|.skip	1000 /* probably overkill*/

/* G MUST BE LAST! */
G:	/* struct globals G; */
