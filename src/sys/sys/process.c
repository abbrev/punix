/*
 * $Id$
 * 
 * Copyright 2005-2008 Christopher Williams
 * 
 * Process management core: switching, signal handling, events, preemption, etc.
 */

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sched.h>

#include "punix.h"
#include "setjmp.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"
#include "process.h"

/* TODO: use setrun when putting a proc in the "running" state */
STARTUP(void setrun(struct proc *p))
{
	p->p_waitchan = NULL;
#if 0
	if (p->p_status == P_RUNNING) {
		kprintf("setrun(): warning: process is already running\n");
		return;
	}
#endif
	sched_run(p);
}

STARTUP(void unsleep(struct proc *p))
{
	p->p_waitchan = 0;
}

#if 1
/* following is from 2.11BSD */

/*
 * Implement timeout for tsleep below.  If process hasn't been awakened
 * (p_waitchan != 0) then set timeout flag and undo the sleep.  If proc
 * is stopped just unsleep so it will remain stopped.
*/

STARTUP(static void endtsleep(void *vp))
{
	struct proc *p = (struct proc *)vp;
	int s;
	
	s = spl7();
	if (p->p_waitchan) {
		if (p->p_status == P_SLEEPING) {
			//kprintf("%s (%d)\n", __FILE__, __LINE__);
			setrun(p);
		} else
			unsleep(p);
		p->p_flag |= P_TIMEOUT;
	}
	splx(s);
}

/* note: following comment block is outdated */
/*
 * General sleep call "borrowed" from 4.4BSD - the 'wmesg' parameter was
 * removed due to data space concerns.  Sleeps at most timo/hz seconds
 * 0 means no timeout). NOTE: timeouts in 2.11BSD use a signed int and 
 * thus can be at most 32767 'ticks' or about 540 seconds in the US with 
 * 60hz power (~650 seconds if 50hz power is being used).
 *
 * If 'pri' includes the PCATCH flag signals are checked before and after
 * sleeping otherwise  signals are not checked.   Returns 0 if a wakeup was
 * done, EWOULDBLOCK if the timeout expired, ERESTART if the current system
 * call should be restarted, and EINTR if the system call should be
 * interrupted and EINTR returned to the user process.
*/

/* intr is 0 if process wants an uninterrutible sleep */
STARTUP(int tsleep(void *chan, int intr, long timo))
{
	struct proc *p = &P;
	int s;
	int sig;

	s = spl7();
#if 0
	if (panicstr) {
/*
 * After a panic just give interrupts a chance then just return.  Don't
 * run any other procs (or panic again below) in case this is the idle
 * process and already asleep.  The splnet should be spl0 if the network
 * was being used but for now avoid network interrupts that might cause
 * another panic.
*/
		(void)_splnet();
		noop();
		splx(s);
		return;
	}
#endif
#ifdef	DIAGNOSTIC
	if (chan == NULL || p->p_status != P_RUNNING)
		panic("tsleep");
#endif
	p->p_waitchan = chan;
	if (timo)
		timeout(endtsleep, p, timo);
/*
 * We put outselves on the sleep queue and start the timeout before calling
 * CURSIG as we could stop there and a wakeup or a SIGCONT (or both) could
 * occur while we were stopped.  A SIGCONT would cause us to be marked SSLEEP
 * without resuming us thus we must be ready for sleep when CURSIG is called.
 * If the wakeup happens while we're stopped p->p_waitchan will be 0 upon 
 * return from CURSIG.
*/
	if (intr) {
		p->p_flag |= P_SINTR;
		if ((sig = CURSIG(p))) {
			if (p->p_waitchan)
				unsleep(p);
			goto resume;
		}
		if (p->p_waitchan == 0) {
			intr = 0;
			goto resume;
		}
	} else {
		p->p_flag &= ~P_SINTR;
		sig = 0;
	}
	/* p->p_status = P_SLEEPING; */
	sched_sleep(p);
	++P.p_rusage.ru_nvcsw;
	swtch();
resume:
	splx(s);
	p->p_flag &= ~P_SINTR;
	if (p->p_flag & P_TIMEOUT) {
		p->p_flag &= ~P_TIMEOUT;
		if (sig == 0)
			return EWOULDBLOCK;
	} else if (timo)
		untimeout(endtsleep, (void *)p);
	if (intr && (sig != 0 || (sig = CURSIG(p)))) {
		if (P.p_sigintr & sigmask(sig))
			return EINTR;
		return ERESTART;
	}
	return 0;
}

#endif

/* sleep on chan */
STARTUP(void slp(void *chan, int intr))
{
	P.p_error = tsleep(chan, intr, (long)0);
	
	if (!intr || P.p_error == 0)
		return;
	
	longjmp(P.p_qsav, 1);
}



STARTUP(void wakeup(void *chan))
{
	struct proc *p;
	
	list_for_each_entry(p, &G.proc_list, p_list)
		if (p->p_waitchan == chan) {
			//kprintf("%s (%d)\n", __FILE__, __LINE__);
			setrun(p);
		}
}

/* allocate a process structure */
STARTUP(struct proc *palloc())
{
	size_t psize = sizeof(struct proc);
	struct proc *p = memalloc(&psize, 0);
	
	if (p) {
		list_add(&p->p_list, &G.proc_list);
	}
	return p;
}

/* free a process structure */
STARTUP(void pfree(struct proc *p))
{
	if (p) {
		/* remove this proc from the list */
		list_del(&p->p_list);
		memfree(p, 0);
	}
}

#define MAXPID	30000
/* find an unused process id */
/* FIXME: make this faster and cleaner */
STARTUP(int pidalloc())
{
	struct proc *p;
	/*
	static int pidchecked = 0;
	static int mpid = 1;
	*/
	
	/*
	 * mpid is the current pid.
	 * pidchecked is the lowest pid after mpid that is currently used by a
	 * process or process group. This is to avoid checking all processes
	 * each time we need a pid.
	 */
	++G.mpid;
retry:
	if (G.mpid >= MAXPID) {
		G.mpid = 2;
		G.pidchecked = 0;
	}
	if (G.mpid >= G.pidchecked) {
		G.pidchecked = MAXPID;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_pid == G.mpid || p->p_pgrp == G.mpid) {
				++G.mpid;
				if (G.mpid >= G.pidchecked)
					goto retry;
			}
			if (G.mpid < p->p_pid && p->p_pid < G.pidchecked)
				G.pidchecked = p->p_pid;
			if (G.mpid < p->p_pgrp && p->p_pgrp < G.pidchecked)
				G.pidchecked = p->p_pgrp;
		}
	}
	
	return G.mpid;
}

/*
 * Is p an inferior of the current process?
 */
STARTUP(int inferior(struct proc *p))
{

        for (; p != current; p = p->p_pptr)
                if (!p->p_pptr)
                        return 0;
        return 1;
}

/* find the process with the given process id, or NULL if none is found */
STARTUP(struct proc *pfind(pid_t pid))
{
	struct proc *p;
	list_for_each_entry(p, &G.proc_list, p_list) {
		if (p->p_pid == pid)
			return p;
	}
	
	P.p_error = ESRCH;
	return NULL;
}

STARTUP(void procinit())
{
	int i;
	
	INIT_LIST_HEAD(&G.proc_list);
	
	current = palloc();
	assert(current);
	G.initproc = current;
	
	for (i = 0; i < NOFILE; ++i)
		P.p_ofile[i] = NULL;
	
	/* construct our first proc (awww, how cute!) */
	P.p_pid = 1;
	P.p_ruid = P.p_euid = P.p_svuid = 0;
	P.p_rgid = P.p_egid = P.p_svgid = 0;
	P.p_status = P_NEW;
	P.p_nice = NZERO;
	P.p_pptr = NULL;
	P.p_sched_policy = SCHED_NORMAL;
	P.p_prio = NORMAL_PRIO;
	
	/* set some resource limits. XXX: put more here! */
	for (i = 0; i < 7; ++i)
		P.p_rlimit[i].rlim_cur = P.p_rlimit[i].rlim_max = RLIM_INFINITY;
	
	G.numrunning = 0;
	G.cumulrunning = 0;
	//kprintf("%s (%d)\n", __FILE__, __LINE__);
	sched_run(current);
}
