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

#include "punix.h"
#include "setjmp.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"
#include "process.h"

/* FIXME: XXX */
STARTUP(void cpuidle(void))
{
	int x = spl0();
	nop();
	splx(x);
}

/* TODO: use setrun when putting a proc in the "running" state */
STARTUP(void setrun(struct proc *p))
{
	void *w;
	
	if (p->p_status == P_RUNNING) return;
	if (p->p_status == P_FREE || p->p_status == P_ZOMBIE)
		panic("Running a dead proc");
	
	if (p->p_status == P_SLEEPING && (w = p->p_waitfor)) {
		p->p_waitfor = 0;
		setpri(p);
	}
	p->p_status = P_RUNNING;
	++G.numrunning;
	if (p->p_pri < P.p_pri)
		++runrun;
}

void setsleep(struct proc *p)
{
	if (p->p_status == P_RUNNING) --G.numrunning;
	p->p_status = P_SLEEPING;
}

/*
 * Set user priority.
 * The rescheduling flag (runrun) is set if the priority is better than the
 * currently running process.
 *
 * This should be called only when the proc's priority is known to have changed.
 */
STARTUP(int setpri(struct proc *p))
{
	int pri;
	pri = p->p_basepri + 64 * (p->p_nice * NICEPRIWEIGHT +
	                           p->p_cputime * CPUPRIWEIGHT) / CPUMAX;
	if (pri > 255)
		pri = 255;
	if (pri < P.p_pri)
		++runrun;
	p->p_pri = pri;
	return pri;
}

/* exponentially decay the cpu time of each process */
STARTUP(void decaycputimes())
{
	struct proc *p;
	
	for EACHPROC(p) {
		p->p_cputime = p->p_cputime * CPUDECAY / CPUSCALE;
		setpri(p);
	}
}

/*
 * This is the scheduler proper. Returns NULL if no process is running.
 * 
 * This scheduler runs in O(n) time (you're welcome to re-write it to make
 * it O(1)).
 */
STARTUP(static struct proc *nextready())
{
	struct proc *p;
	struct proc *bestp = NULL;
	int bestpri = 255;
	
	for EACHPROC(p) {
		if (p->p_status != P_RUNNING)
			continue;
		
		if (p->p_pri < bestpri) {
			bestpri = p->p_pri;
			bestp = p;
		}
	}
	
	return bestp;
}

STARTUP(void swtch())
{
	struct proc *p;
	int t;
	
	spl1(); /* higher than 256Hz timer */
	
	/* XXX: this shows the number of times this function has been called.
	 * It draws in the bottom-right corner of the screen.
	 */
#if 0
	++*(long *)(0x4c00+0xf00-4);
#endif
	
	/*
	 * When a process switches between clock ticks, we keep track of this
	 * by adding one-half of a clock tick to its cpu time. This is based
	 * on the theory that a process will use, on average, half of a clock
	 * tick when it switches between ticks. To balance the accounting, we
	 * subtract one-half clock tick from the cpu time of the next process
	 * coming in if it doesn't get a full first clock tick. Doing this will
	 * (hopefully) more correctly account for the cpu usage of processes
	 * which always switch (sleep) before the first clock tick of their
	 * time slice. The correct solution, of course, requires using a
	 * high-precision clock that tells us exactly how much cpu time the
	 * process uses, but we don't have a high-resolution clock. :(
	 */
	
	t = 0;
	
	if (!istick) {
		/* we switched between ticks, so add one-half clock tick to our
		 * cpu time and subtract one from the next proc's cpu time */
		++cputime;
		--t;
	}
	
	P.p_cputime += cputime * CPUSCALE / 2;
	P.p_basepri = PUSER;
	setpri(current);
	while (P.p_cputime >= CPUMAX)
		decaycputimes();
	
	while (!(p = nextready())) {
		struct proc *pp = current;
		current = NULL; /* don't bill any process if they're all asleep */
		cpuidle();
		current = pp;
		t = 0; /* the next proc will start on a clock tick */
	}
	
	cputime = t;
	runrun = 0;
	istick = 0;
	
	if (p == &P)
		return;
	
	if (!P.p_fpsaved) {
		/* savefp(&P.p_fps); */
		P.p_fpsaved = 1;
	}
	
	if (setjmp(P.p_ssav))
		return; /* we get here via longjmp */
	
	current = p;
	
	longjmp(P.p_ssav, 1);
}

STARTUP(void unsleep(struct proc *p))
{
	int x = spl7();
	
	if (p->p_waitfor)
		p->p_waitfor = 0;
	
	splx(x);
}

#if 1
/* following is from 2.11BSD */

/*
 * Implement timeout for tsleep below.  If process hasn't been awakened
 * (p_waitfor != 0) then set timeout flag and undo the sleep.  If proc
 * is stopped just unsleep so it will remain stopped.
*/

STARTUP(static void endtsleep(void *vp))
{
	struct proc *p = (struct proc *)vp;
	int s;
	
	s = spl7();
	if (p->p_waitfor) {
		if (p->p_status == P_SLEEPING)
			setrun(p);
		else
			unsleep(p);
		p->p_flag |= P_TIMEOUT;
	}
	splx(s);
}

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

STARTUP(int tsleep(void *event, int basepri, long timo))
{
	struct proc *p = &P;
	int s;
	int sig, catch = basepri & PCATCH;

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
	if (event == NULL || p->p_status != P_RUNNING)
		panic("tsleep");
#endif
	p->p_waitfor = event;
#if 0
	p->p_slptime = 0;
#endif
	p->p_basepri = basepri & PRIMASK;
	if (timo)
		timeout(endtsleep, p, timo);
/*
 * We put outselves on the sleep queue and start the timeout before calling
 * CURSIG as we could stop there and a wakeup or a SIGCONT (or both) could
 * occur while we were stopped.  A SIGCONT would cause us to be marked SSLEEP
 * without resuming us thus we must be ready for sleep when CURSIG is called.
 * If the wakeup happens while we're stopped p->p_waitfor will be 0 upon 
 * return from CURSIG.
*/
	if (catch) {
		p->p_flag |= P_SINTR;
		if ((sig = CURSIG(p))) {
			if (p->p_waitfor)
				unsleep(p);
			p->p_status = P_RUNNING; /* FIXME: use setrun(p); */
			goto resume;
		}
		if (p->p_waitfor == 0) {
			catch = 0;
			goto resume;
		}
	} else
		sig = 0;
	/* p->p_status = P_SLEEPING; */
	setsleep(p);
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
	if (catch && (sig != 0 || (sig = CURSIG(p)))) {
		if (P.p_sigintr & sigmask(sig))
			return EINTR;
		return ERESTART;
	}
	return 0;
}

#endif

/* sleep until event */
STARTUP(void slp(void *event, int basepri))
{
	int priority = basepri;
	if (basepri > PZERO)
		priority |= PCATCH;
	P.p_error = tsleep(event, priority, (long)0);
	
	if (!(priority & PCATCH) || P.p_error == 0)
		return;
	
	longjmp(P.p_qsav, 1);
}



STARTUP(void wakeup(void *event))
{
	struct proc *p;
	
	for EACHPROC(p) {
		if (p->p_status == P_SLEEPING && p->p_waitfor == event) {
			setrun(p);
		}
	}
}

/* allocate a process structure */
STARTUP(struct proc *palloc())
{
	size_t psize = sizeof(struct proc);
	struct proc *p = memalloc(&psize, 0);
	
	if (p) {
		p->p_next = G.proclist;
		G.proclist = p;
	}
	return p;
}

/* free a process structure */
STARTUP(void pfree(struct proc *p))
{
	if (p) {
		/* remove this proc from the list */
		if (G.proclist == p) {
			G.proclist = p->p_next;
		} else {
			struct proc *p2;
			for EACHPROC(p2) {
				if (p2->p_next == p) {
					p2->p_next = p->p_next;
					break;
				}
			}
		}
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
		
		for EACHPROC(p) {
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
	for EACHPROC(p)
		if (p->p_status != P_FREE && p->p_pid == pid)
			return p;
	
	return NULL;
}

STARTUP(void procinit())
{
	int i;
	
	current = palloc();
	assert(current);
	current->p_next = NULL;
	G.initproc = current;
	
	for (i = 0; i < NOFILE; ++i)
		P.p_ofile[i] = NULL;
	
	/* construct our first proc (awww, how cute!) */
	P.p_pid = 1;
	P.p_ruid = P.p_euid = P.p_svuid = 0;
	P.p_rgid = P.p_egid = P.p_svgid = 0;
	P.p_status = P_RUNNING;
	P.p_cputime = 0;
	P.p_nice = NZERO;
	P.p_basepri = PUSER;
	P.p_pptr = NULL;
	
	/* set some resource limits. XXX: put more here! */
	for (i = 0; i < 7; ++i)
		P.p_rlimit[i].rlim_cur = P.p_rlimit[i].rlim_max = RLIM_INFINITY;
	
	setpri(current);
	cputime = 0;
	G.numrunning = 1;
	G.cumulrunning = 0;
}
