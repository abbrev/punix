/*
 * Punix, Puny Unix kernel
 * Copyright 2005 Chris Williams
 * 
 * $Id: proc.h,v 1.13 2008/04/18 03:44:57 fredfoobar Exp $
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
 * Copyright (c) 1987,1997, Prentice Hall
 * All rights reserved.
 * 
 * Redistribution and use of the MINIX operating system in source and
 * binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 * 
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 * 
 *    * Neither the name of Prentice Hall nor the names of the software
 *      authors or contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS, AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PRENTICE HALL OR ANY AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

/*#include "type.h"*/
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <stdint.h>
#include <setjmp.h>

/* #include "procglo.h" */
#include "param.h"
#include "dir.h"
#include "file.h"
/* #include "signal.h" */
#include "ktime.h"

#if 0
#define NPROC	32
#define NGROUPS	16
#endif

/* "P" is roughly equivalent to "u" in BSD */
#define CURRENT		G.current
#define P		(*CURRENT)

/* this determines whether system calls read their arguments from the user stack
 * or from an array in the proc structure. 0 => array, !0 => USP
 * advantages of USP:
 *  - faster (no copying from stack to proc structure)
 *  - don't need memory for syscall arguments in the proc structure
 * advantages of no USP:
 *  - possibly safer (copyin() can check for out-of-bounds stack pointer)
 * 
 * The only performance advantage of no USP is only if copyin() checks bounds of
 * SP, and that can be relatively slow without an MMU.
 * 
 * Note! Make sure that system calls do not overwrite arguments.
 */
#define USPARGS 1

typedef long register_t;		/* machine register */

struct stackframe {
	register_t retreg /*d0*/, d1, d2, d3, d4, d5, d6, d7;
	register_t a0, a1, a2, a3, a4, a5, fp /*a6*/, sp /*a7*/;
	register_t pc;
	uint16_t psw;
	uint16_t dummy;		/* make size multiple of reg_t for system.c */
};

struct proc {
	struct proc *p_next;
	char p_status;	/* stopped, ready, running, zombie, etc. */
	int p_flag;
	
	/* scheduling */
	int p_nice;
	int p_cputime;	/* amount of cpu time we are using (decaying time) */
	int p_pri;	/* run priority, calculated from cpuusage and nice */
	int p_basepri;	/* base priority */
	
	/* execution state */
	jmp_buf p_ssav;	/* for swapping procs */
	jmp_buf p_qsav;	/* for interrupting syscalls */
	int p_fpsaved;	/* floating-point state is saved? */
	/* ??? p_fps; -- floating-point state */
	
	struct trapframe *p_tfp;	/* trap frame pointer for vfork(2) */
	
	/* segments of user memory (note: the heap is global) */
	void *p_ustack;
	void *p_stack;
	void *p_text;
	void *p_data;
	
	size_t p_ustacksize;
	size_t p_stacksize;
	size_t p_textsize;
	size_t p_datasize;
	
	struct rusage p_rusage;
	struct rlimit p_rlimit[7]; /* CONSTANT */
	
	/* id */
	pid_t p_pid;		/* process id */
	pid_t p_pgrp;		/* process group */
	struct proc *p_pptr;	/* parent proc */
	
	uid_t p_uid;		/* effective user id */
	uid_t p_svuid;		/* saved user id */
	uid_t p_ruid;		/* real user id */
	gid_t p_gid;		/* effective group id */
	gid_t p_rgid;		/* real group id */
	gid_t p_svgid;		/* saved group id */
	gid_t p_groups[NGROUPS];	/* groups, 0 terminated */
	
	/* signals */
#if 0
	sighandler_t p_signal[NSIG];	/* signal handlers */
#else
	struct sigaction p_sigaction[NSIG];
#endif
	sigset_t p_sigmask;	/* correct type ? */
	sigset_t p_sigonstack;	/* signals to take on sigstack */
	sigset_t p_sigintr;	/* signals that interrupt syscalls */
	stack_t p_sigstk;	/* signal stack */
	
	/* syscalls */
#if USPARGS
	void *p_arg;	/* args to syscall on user stack */
#else
	int p_arg[8];	/* args to syscall (probably don't need 8 words) */
#endif
	uint32_t p_retval;
	int p_error;
	
	/* files */
	struct file *p_ofile[NOFILE];	/* file structures for open files */
	char p_oflag[NOFILE];	/* flags of open files */
	int p_lastfile;
	struct inode *p_cdir;	/* current directory */
	struct inode *p_rdir;	/* root directory */
	struct tty *p_ttyp;	/* controlling tty pointer */
	dev_t p_ttydev;		/* controlling tty device */
	
	/* for I/O */
	char *p_base;
	size_t p_count;
	off_t p_offset; /* need this? */
	char p_dbuf[NAME_MAX+1]; /* need this? */
	const char *p_dirp; /* need this? */
	struct inode *p_pdir;
	struct direct p_dent;
	
	
	mode_t p_cmask;		/* file creation mask */
	
#if 0
	/* FIXME */
	/* timing */
	clock_t user_time;	/* user time in ticks */
	clock_t sys_time;	/* sys time in ticks */
	clock_t child_utime;	/* cumulative user time of children */
	clock_t child_stime;	/* cumulative sys time of children */
	clock_t p_alarm;	/* time of next alarm in ticks, or 0 */
	struct itimerval p_itimer_virtual; /* virtual interval timer */
	struct itimerval p_itimer_prof; /* profile interval timer */
#endif
	struct itimerspec p_itimer[3]; /* REAL, VIRTUAL, and PROF timers */
	
#if 0
//	struct proc *p_nextready;	/* pointer to next ready process */
	sigset_t p_pending;	/* bit map for pending signals */
//	unsigned p_pendcount;	/* count of pending and unfinished signals */
#endif
	
	void *p_waitfor;
	
	char p_name[16];	/* name of the process */
	
	/* zombie */
	int p_xstat;		/* exit status */
};

/* states for p_status */
/* are these all good? */
#define P_FREE		0	/* slot is empty */
#define	P_FORKING	1	/* process is blocked by child */
#define P_RUNNING	2
#define P_SLEEPING	3	/* waiting for an event */
#define P_STOPPED	4	/**/
#define P_ZOMBIE	5
#define P_VFORKING	6

/* flags for p_flag */
#define P_TRACED  001 /* process is being traced */
#define P_WAITED  002 /* set if parent received this proc's status already */
#define P_TIMEOUT 004 /* tsleep timeout */

#endif /* _SYS_PROC_H_ */
