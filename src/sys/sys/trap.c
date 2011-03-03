#include <signal.h>
#include <sys/time.h>

#include "punix.h"
#include "process.h"
#include "param.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* These are all called from interrupts in entry.s */

STARTUP(void bus_error())
{
	psignal(&P, SIGBUS);
}

STARTUP(void spurious())
{
}

STARTUP(void address_error())
{
	panic("address error");
	psignal(&P, SIGBUS);
}

STARTUP(void illegal_instr())
{
	psignal(&P, SIGILL);
}

STARTUP(void zero_divide())
{
	psignal(&P, SIGFPE);
}

STARTUP(void chk_instr())
{
}

STARTUP(void i_trapv())
{
}

STARTUP(void privilege())
{
	psignal(&P, SIGILL);
}

STARTUP(void trace())
{
}

/* NB: user time is in ticks */
#define BUMPUTIME(tv, ticks) do { \
	(tv)->tv_usec += (ticks); \
	if ((tv)->tv_usec >= HZ) { \
		(tv)->tv_usec -= HZ; \
		(tv)->tv_sec++; \
	} \
} while (0)

#define BUMPNTIME(tv, nsec) do { \
	(tv)->tv_nsec += (nsec); \
	if ((tv)->tv_nsec >= SECOND) { \
		(tv)->tv_nsec -= SECOND; \
		(tv)->tv_sec++; \
	} \
} while (0)

/* SLEWRATE is in microseconds per tick */
/* 512 us = 0.512 ms */
#define SLEWRATE (512 / HZ)
#define BIGADJ 1000000L /* 1 s */
#define BIGSLEWRATE (5000 / HZ)

STARTUP(void hardclock(unsigned short ps))
{
	int itimerdecr(struct itimerspec *itp, long nsec);
	int sig;
	int whereami;
	
	splclock();
	
	whereami = G.whereami;
	++G.ticks;

	if (current && ++G.spin >= 2) {
		unsigned long *spinner = (unsigned long *)(0x4c00+0xf00-7*30+whereami*4);
		if (*spinner)
			*spinner = RORL(*spinner, 1);
		else
			*spinner = 0x0fffffff;
		G.spin = 0;
	}
	
#if 0
	/* XXX: this shows the number of times this function has been called.
	 * It draws in the bottom-right corner of the screen.
	 */
	++*(long *)(0x4c00+0xf00-8);
#endif
	
#if 1
	if (timeadj) {
		/* slow down or speed up the clock by a small amount */
		long delta;
		if (timeadj < -BIGADJ || BIGADJ < timeadj) {
			delta = BIGSLEWRATE;
		} else if (timeadj < -SLEWRATE || SLEWRATE < timeadj) {
			delta = SLEWRATE;
		} else {
			delta = timeadj < 0 ? -timeadj : timeadj;
		}
		if (timeadj < 0) delta = -delta;
		G.timeoffset.tv_nsec += delta * 1000;
		timeadj -= delta;
		
		if (G.timeoffset.tv_nsec >= SECOND) {
			G.timeoffset.tv_nsec -= SECOND;
			++G.timeoffset.tv_sec;
		} else if (G.timeoffset.tv_nsec < 0) {
			G.timeoffset.tv_nsec += SECOND;
			--G.timeoffset.tv_sec;
		}
	}
#endif
	
	BUMPNTIME(&uptime, TICK);
	G.cumulrunning += G.numrunning;
	++loadavtime;
	
	if (loadavtime >= HZ * 5) {
#if 0
		/* sanity check */
		struct proc *p;
		int i;
		int n = 0;
		int x = spl7();
		for (i = 0; i < PRIO_LIMIT; ++i)
			list_for_each_entry(p, &G.runqueues[i], p_runlist)
				++n;
		if (G.numrunning != n) {
			kprintf("warning: numrunning=%d n=%d\n",
				G.numrunning, n);
			G.numrunning = n;
		}
		splx(x);
#endif
		loadav((unsigned long)G.cumulrunning * F_ONE / loadavtime);
		
		batt_check();
		loadavtime = 0;
		G.cumulrunning = 0;
#if 0
		++*(long *)(0x4c00+0xf00-26);
#endif
	}
	
	sched_tick();
	
	if (current) {
		if (timespecisset(&P.p_itimer[ITIMER_PROF].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_PROF], TICK))
			psignal(current, SIGPROF);
	}
	
	if (USERMODE(ps)) {
		BUMPUTIME(&P.p_rusage.ru_utime, 1);
		if (timespecisset(&P.p_itimer[ITIMER_VIRTUAL].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_VIRTUAL], TICK))
			psignal(current, SIGVTALRM);
	} else {
		if (current)
			BUMPUTIME(&P.p_rusage.ru_stime, 1);
	}
	
	/* do call-outs */
	
	if (G.callout[0].c_func == NULL)
		goto out;
	
	--G.callout[0].c_dtime;
	
	if (G.calloutlock)
		goto out;
	
	++G.calloutlock;
	spl0();
	
	if (G.callout[0].c_dtime <= 0) {
		int t = 0;
		struct callout *c1, *c2;
		c1 = c2 = &G.callout[0];
		do {
			c2->c_func(c2->c_arg);
			++c2;
			t += c2->c_dtime;
		} while (c2->c_func != NULL && t <= 0);
		do
			*c1 = *c2++;
		while (c1++->c_func);
	}
	G.calloutlock = 0;
	
out:	spl0();
	
	scankb();
	
	spl1();
	
	if (!USERMODE(ps)) return;
	
	/* preempt a running user process */
	if (G.need_resched) {
		swtch();
	}
	
	while ((sig = CURSIG(&P)))
		postsig(sig);
}

#if 1
/*
 * RTC interrupt handler. This runs even when the calc is off, when hardclock()
 * does not run.
 */
STARTUP(void updwalltime())
{
	/* the tv_nsec field of walltime maintains the "0" time of G.ticks%HZ */
	++walltime.tv_sec;
	walltime.tv_nsec = (G.ticks+1) % HZ;
}
#endif
