/*
 * Punix, Operating System for TI-89/TI-92+/V200
 * Copyright 2009 Chris Williams
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

.section _st1,"x"
SYS_exit         = 1	/* put these in a header please */
SYS_read         = 3
SYS_write        = 4
SYS_open         = 5
SYS_close        = 6
SYS_kmalloc      = 17
SYS_kfree        = 18
SYS_getpid       = 20
SYS_uname        = 23
SYS_getuid       = 24
SYS_getppid      = 27
SYS_sysctl       = 30
SYS_sigaction    = 31
SYS_kill         = 37
SYS_dup          = 41
SYS_getgid       = 47
SYS_ioctl        = 54
SYS_execve       = 59
SYS_pause        = 63
SYS_vfork        = 66
SYS_getgroups    = 79
SYS_getpgrp      = 81
SYS_setitimer    = 83
SYS_wait         = 87
SYS_getpriority  = 96
SYS_gettimeofday = 116
SYS_getrusage    = 117
SYS_settimeofday = 122
SYS_adjtime      = 140
SYS_getloadavg1  = 160
SYS_poweroff = 69

.macro	sys call
	move	#SYS_\call,%d0
	trap	#0
.endm

.macro	mksyscall name
	.global \name
\name:
	sys	\name
	bcs	cerror
	rts
.endm

.macro	mksyscalla name
	.global \name
\name:
	sys	\name
	bcs	caerror
	move.l	%d0,%a0
	rts
.endm

.even
	.long	0xbeef0001	| for debugging ;)
	
	.global _exit
_exit:
	sys	exit
0:	nop
	bra.s	0b

mksyscall	open
mksyscall	close
mksyscall	read
mksyscall	write
mksyscall	execve
mksyscall	setitimer
mksyscall	getpgrp
mksyscall	gettimeofday
mksyscall	settimeofday
mksyscall	getrusage
mksyscall	getpid
mksyscall	uname
mksyscall	getppid
mksyscall	sigaction
mksyscall	dup
mksyscall	getpriority
mksyscall	getloadavg1
mksyscalla	kmalloc
mksyscall	kfree
mksyscall	ioctl
mksyscall	getuid
mksyscall	getgid
mksyscall	getgroups
mksyscall	wait
|mksyscall	vfork
mksyscall	pause
mksyscall	adjtime
mksyscall	poweroff
mksyscall	sysctl
mksyscall	kill

.global cerror, caerror
/*
 * common routine to handle errors from system calls: set errno appropriately
 * and return -1.
 */
cerror:
	| move	%d0,errno
	move	%d0,-(%sp)
	jbsr	seterrno
	lea.l	(2,%sp),%sp
	moveq.l	#-1,%d0
	rts

caerror:
	| move	%d0,errno
	move	%d0,-(%sp)
	jbsr	seterrno
	lea.l	(2,%sp),%sp
	sub.l	%a0,%a0
	rts

.macro mkstart name
.global \name\()_start
\name\()_start:
	move	(%sp),%d0		| argc
	lea	2(%sp),%a0		| argv
	move	%d0,%d1
	mulu	#4,%d1
	lea	6(%sp,%d1.w),%a1	| env
	
	move.l	%a1,-(%sp)	| char **env
	move.l	%a0,-(%sp)	| char **argv
	move	%d0,-(%sp)	| int argc
	jbsr	\name\()_main	| \name\()_main(argc, argv, env)
	
	move	%d0,-(%sp)
	bsr	_exit
	bra	.
.endm

mkstart	init
mkstart bittybox
mkstart sh
mkstart time
mkstart getty
mkstart login
mkstart uterm

	.global fadd
| float fadd(float a, float b);
fadd:
	fmove.s	(4,%sp),%fp0
	fadd.s	(8,%sp),%fp0
	fmove.s	%fp0,%d0
	rts

printfptype:
	movem.l	%a0-%a3,-(%sp)
	
	| test for nan
	fbun	1f
	
	| test for zero/not zero
	fbne	2f
	pea.l	4f	| 0
	bra	3f
2:	pea.l	5f	| x
0:	|bra	3f
	
	| test for sign (doesn't recognize negative zero)
3:	fblt	2f
	pea.l	7f	| +
	bra	0f
2:	pea.l	6f	| -
	
0:	pea.l	8f
	
	jbsr	printf
	lea.l	(3*4,%sp),%sp
	
	movem.l	(%sp)+,%a0-%a3
	rts

1:	pea.l	9f	| nan
	pea.l	9f-1	| (empty string)
	bra	0b


4:	.asciz "0"
5:	.asciz "x"
6:	.asciz "-"
7:	.asciz "+"
8:	.asciz "%s%s\n"
9:	.asciz "nan"

	.global fputest
fputest:
	move.l	#0x7fc00000,%d5
	
	fneg.b	#42,%fp7
	bsr	printfptype
	fneg.w	#42,%fp7
	bsr	printfptype
	|fneg.l	#42,%fp7

	|fneg.s	#0x7fc00000  | nan (single) [invalid format?]
	| above instruction would assemble to this:
	.word 0xf23c,0x479a,0x7fc0,0x0000
	bsr	printfptype
	fneg.x	%d5,%fp7
	fneg.b	#-42,%fp7
	bsr	printfptype
	fneg.w	#-42,%fp7
	bsr	printfptype
	fneg.l	#-42,%fp7
	bsr	printfptype
	fneg.b	#0,%fp7
	bsr	printfptype
	fneg.w	#0,%fp7
	bsr	printfptype
	fneg.l	#0,%fp7
	bsr	printfptype
	rts
	
	| %Dn.b
	fneg.b	%d5,%fp7	| 42.b
	bsr	printfptype
	
	| %Dn.w
	fneg.w	%d5,%fp7	| 42.w
	bsr	printfptype
	
	| (d16,%pc)
	fneg.x	(11f,%pc),%fp7	| nan
	bsr	printfptype
	
	fneg.p	(11f+12,%pc),%fp7
	bsr	printfptype
	
	| (An)
	lea	12f,%a3
	fneg.d	(%a3),%fp3	| nan (double)
	bsr	printfptype
	
	| (An)+
	fneg.d	(%a3)+,%fp5	| nan (double)
	bsr	printfptype
	fneg.d	(%a3)+,%fp1	| 42 (double)
	bsr	printfptype
	fneg.s	(%a3)+,%fp7	| nan (single)
	bsr	printfptype
	fneg.s	(%a3)+,%fp0	| 42 (single)
	bsr	printfptype
	fneg.l	(%a3)+,%fp6	| 42.l
	bsr	printfptype
	fneg.w	(%a3)+,%fp7	| 42.w
	bsr	printfptype
	fneg.b	(%a3)+,%fp3	| 0.b
	bsr	printfptype
	
	| -(An)
	fneg.b	-(%a3),%fp4	| 0.b
	bsr	printfptype
	fneg.w	-(%a3),%fp7	| 42.w
	bsr	printfptype
	fneg.l	-(%a3),%fp7	| 42.l
	bsr	printfptype
	fneg.s	-(%a3),%fp7	| 42 (single)
	bsr	printfptype
	fneg.s	-(%a3),%fp7	| nan (single)
	bsr	printfptype
	fneg.d	-(%a3),%fp7	| 42 (double)
	bsr	printfptype
	fneg.d	-(%a3),%fp5	| nan (double)
	bsr	printfptype
	
	rts
11:	.long 0x7fff0000,0x40000000,0x00000000  | nan
	.long 0x00000000,0x00000000,0x00000000  | ?? (packed decimal)
12:	.long 0x7ff80000,0x00000000  | nan (double)
	.long 0x40450000,0x00000000  | 42 (double)
	.long 0x7fc00000  | nan (single)
	.long 0x42280000  | 42 (single)
	.long 42
	.word 42
	.byte 0
