/*
 * Punix, Puny Unix kernel
 * Copyright 2005 Chris Williams
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

/* #include "procglo.h" */
#include "param.h"
#include "dir.h"
#include "file.h"
/* #include "signal.h" */
#include "ktime.h"
#include "setjmp.h"
#include "list.h"
#include "context.h"

#if 0
#define NPROC	32
#define NGROUPS	16
#endif

/* "P" is roughly equivalent to "u" in BSD */
#define P		(*current)

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

#if 0
typedef long register_t;		/* machine register */

struct stackframe {
	register_t retreg /*d0*/, d1, d2, d3, d4, d5, d6, d7;
	register_t a0, a1, a2, a3, a4, a5, fp /*a6*/, sp /*a7*/;
	register_t pc;
	uint16_t psw;
	uint16_t dummy;		/* make size multiple of reg_t for system.c */
};
#endif

struct krusage {
	clock_t kru_utime;
	clock_t kru_stime;
	long    kru_nvcsw;
	long    kru_nsignals;
};

#define P_NAMELEN 16

/*
 * struct zombproc contains only those fields needed by a zombie process to
 * minimize memory consumed by such processes. The fields match up with the
 * first fields in struct proc for obvious reasons.
 */
struct proc {
	struct zombproc {
		struct list_head p_list;
		char p_status;	/* stopped, ready, running, zombie, etc. */

		/* our resource usage */
		struct krusage p_kru;
		pid_t p_pid;		/* process id */
		pid_t p_pgrp;		/* process group */
		struct proc *p_pptr;	/* parent proc */
		
		int p_waitstat;		/* status for wait() */
		
		char p_name[P_NAMELEN];	/* name of the process */
	};

	/* children cumulative resource usage */
	struct krusage p_ckru;
	struct rlimit p_rlimit[7]; /* CONSTANT */
	
	/* id */
	uid_t p_ruid;		/* real user id */
	uid_t p_euid;		/* effective user id */
	uid_t p_svuid;		/* saved user id */
	gid_t p_rgid;		/* real group id */
	gid_t p_egid;		/* effective group id */
	gid_t p_svgid;		/* saved group id */
	gid_t p_groups[NGROUPS];	/* groups, -1 terminated */
	
	
	int p_flag;
	
	/* syscalls */
#ifdef USPARGS
#if USPARGS
	void *p_arg;	/* args to syscall on user stack */
#else
	int p_arg[11];	/* args to syscall */
#endif
#else
#error "USPARGS is not defined!"
#endif
	struct context *p_syscall_ctx; /* context for execve(2) and vfork(2) */
	uint32_t p_retval;
	int p_error;
	
	/* scheduling */
	unsigned p_cputime;/* amount of cpu time we are using (decaying time) */
	int p_pctcpu; /* percentage of cpu usage for top/ps (8:8 fixed point) */
	//int p_pri;	/* run priority, calculated from cpuusage and nice */
	/* the above might not be needed anymore with the new scheduler */
	struct list_head p_runlist;
	int p_sched_policy;
	int p_prio;       /* this determines which runqueue this proc runs in */
	int p_nice;       /* nice is always represented as 0..39 internally */
	int p_static_prio;
	int p_time_slice;
	int p_first_time_slice;
	unsigned long p_deadline;
	
	/* execution state */
	struct context p_ctx; /* process context when switching procs */
	int p_fpsaved;	/* floating-point state is saved? */
	/* ??? p_fps; -- floating-point state */
	
	/* segments of user memory (note: the heap is global) */
	void *p_ustack;
	void *p_stack;
	void *p_text;
	void *p_data;
	
#if 0
	/* are these really needed? */
	size_t p_ustacksize;
	size_t p_stacksize;
	size_t p_textsize;
	size_t p_datasize;
#endif
	
	/* signals */
	/* 2.11BSD => Punix
	 * u_signal[]() => p_signal[]()
	 * u_sigmask[]  => p_sigmasks[]
	 * u_sigonstack => p_sigonstack
	 * u_sigintr    => p_sigintr
	 * 
	 * u_oldmask    => p_oldmask
	 * u_code       => p_sigcode
	 * u_psflags    => p_psflags
	 * u_sigstk     => p_sigstk
	 * p_sig        => p_sig
	 * p_sigmask    => p_sigmask
	 * p_sigignore  => p_sigignore
	 * p_sigcatch   => p_sigcatch
	 */
	jmp_buf p_sigjmp;	/* for interrupting syscalls */
	sigset_t p_sig;		/* currently posted signals */
	sigset_t p_sigmask;	/* current signal mask */
	sigset_t p_oldmask;	/* saved mask from before sigpause */
	sigset_t p_sigignore;	/* signals being ignored */
	sigset_t p_sigcatch;	/* signals being caught by user */
	int p_sigcode;		/* code to trap */
	char p_psflags;		/* process signal flags */
	char p_ptracesig;	/* */
	struct sigaltstack p_sigstk;	/* signal stack */
	
	/* expansion of struct sigaction */
	void (*p_signal[NSIG])();	/* signal handlers */
	sigset_t p_sigmasks[NSIG];	/* signal masks */
	sigset_t p_sigonstack;		/* signals to run on alternate stack */
	sigset_t p_sigintr;		/* signals that interrupt syscalls */
	sigset_t p_signocldwait;
	
	/* file descriptors */
	struct file *p_ofile[NOFILE];	/* file structures for open files */
#if 0
	char p_oflag[NOFILE];	/* flags of open files */
#else
	struct oflag {
		char cloexec[(NOFILE+7)/8];
		/* add more fields here if we need more flags */
	} p_oflag;
#endif
	int p_lastfile;
	struct inode *p_cdir;	/* current directory */
	struct inode *p_rdir;	/* root directory */
	struct tty *p_ttyp;	/* controlling tty pointer */
	dev_t p_ttydev;		/* controlling tty device */

	struct inode *p_tty;	/* controlling tty */
	
	mode_t p_cmask;		/* file creation mask */
	
	/* for I/O */
	char *p_base;
	size_t p_count;
	off_t p_offset; /* need this? */
#if 0
	char p_dbuf[NAME_MAX+1]; /* need this? */
#endif
	const char *p_dirp; /* need this? */
	struct inode *p_pdir;
	struct direct p_dent;
	
	struct itimerspec p_itimer[3]; /* REAL, VIRTUAL, and PROF timers */
	
	void *p_waitchan;
	
	struct nameicache {		/* last successful directory search */
		off_t nc_prevoffset;	/* offset at which last entry found */
		ino_t nc_inumber;	/* inum of cached directory  */
		dev_t nc_dev;		/* dev of cached directory */
	} p_ncache;
};

/* states for p_status */
/* are these all good? */
#define P_NEW		0	/* new process (only used by system init) */
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
#define P_SINTR   010 /* a signal can interrupt sleep */
#define P_NOCLDWAIT 020 /* don't wait for children when they terminate */
#define P_NOCLDSTOP 040 /* don't receive notification of stopped children */
#define P_VFORK     0100 /* vforking */
#define P_VFDONE    0200 /* end vfork */

#endif /* _SYS_PROC_H_ */
