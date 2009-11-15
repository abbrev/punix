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
SYS_sigaction    = 31
SYS_dup          = 41
SYS_getgid       = 47
SYS_ioctl        = 54
SYS_execve       = 59
SYS_pause        = 63
SYS_vfork        = 66
SYS_getgroups    = 79
SYS_setitimer    = 83
SYS_wait         = 87
SYS_getpriority  = 96
SYS_gettimeofday = 116
SYS_getrusage    = 117
SYS_settimeofday = 122
SYS_adjtime      = 140
SYS_getloadavg1  = 160

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
0:	bra.s	0b

mksyscall	open
mksyscall	close
mksyscall	read
mksyscall	write
mksyscall	execve
mksyscall	setitimer
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
mksyscall	vfork
mksyscall	pause
mksyscall	adjtime

/*
 * common routine to handle errors from system calls: set errno appropriately
 * and return -1.
 */
cerror:
	| move	%d0,errno
	moveq.l	#-1,%d0
	rts

caerror:
	| move	%d0,errno
	move.l	#0,%a0
	rts

.macro mkstart name
.global start_\name
start_\name:
	move	(%sp),%d0		| argc
	lea	2(%sp),%a0		| argv
	move	%d0,%d1
	mulu	#4,%d1
	lea	6(%sp,%d1.w),%a1	| env
	
	move.l	%a1,-(%sp)	| char **env
	move.l	%a0,-(%sp)	| char **argv
	move	%d0,-(%sp)	| int argc
	jbsr	main_\name	| main_\name(argc, argv, env)
	
	move	%d0,-(%sp)
	bsr	_exit
	bra	.
.endm

mkstart	init
mkstart	top
mkstart	usertest

