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

#define BUMPUTIME(tv, usec) do { \
	(tv)->tv_usec += (usec); \
	if ((tv)->tv_usec >= 1000000L) { \
		(tv)->tv_usec -= 1000000L; \
		(tv)->tv_sec++; \
	} \
} while (0)

#define BUMPNTIME(tv, nsec) do { \
	(tv)->tv_nsec += (nsec); \
	if ((tv)->tv_nsec >= 1000000000L) { \
		(tv)->tv_nsec -= 1000000000L; \
		(tv)->tv_sec++; \
	} \
} while (0)

/* SKEW is in microseconds per tick */
/* 2560 us = 2.56 ms */
#define SKEW (2560 / HZ)

STARTUP(void hardclock(unsigned short ps))
{
	struct callout *c1, *c2;
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
	
	realtime.tv_nsec += TICK;
#if 0
	if (timedelta) {
		long delta;
		if (timedelta < -SKEW) {
			delta = SKEW;
			*(long *)(0x4c00+0xf00-20) = 0x888888ff;
		} else if (SKEW < timedelta) {
			delta = -SKEW;
			*(long *)(0x4c00+0xf00-20) = 0xff111111;
		} else {
			delta = timedelta;
			*(long *)(0x4c00+0xf00-20) = 0xffffffff;
		}
		realtime.tv_nsec += delta * 1000;
		walltime.tv_nsec += delta * 1000;
		timedelta -= delta;
		
		if (walltime.tv_nsec >= SECOND) {
			walltime.tv_nsec -= SECOND;
			++walltime.tv_sec;
		} else if (walltime.tv_nsec < 0) {
			walltime.tv_nsec += SECOND;
			--walltime.tv_sec;
		}
	} else
		*(long *)(0x4c00+0xf00-20) = 0xffffffff;
	*(long *)(0x4c00+0xf00-20-30) = walltime.tv_nsec;
	*(long *)(0x4c00+0xf00-20-30-30) = timedelta;
#endif
	if (realtime.tv_nsec >= SECOND) {
		realtime.tv_nsec -= SECOND;
		++realtime.tv_sec;
	}
	G.lbolt = 0;
	
	BUMPNTIME(&uptime, TICK);
	G.cumulrunning += G.numrunning;
	++loadavtime;
	
	if (loadavtime >= HZ * 5) {
		struct proc *p;
		int n = 0;
		list_for_each_entry(p, &G.runqueue, p_runlist)
			++n;
		/* sanity check */
		if (G.numrunning != n) {
			kprintf("warning: numrunning=%d n=%d\n",
				G.numrunning, n);
			G.numrunning = n;
		}
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
		BUMPUTIME(&P.p_rusage.ru_utime, TICK / 1000);
		if (timespecisset(&P.p_itimer[ITIMER_VIRTUAL].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_VIRTUAL], TICK))
			psignal(current, SIGVTALRM);
	} else {
		if (current)
			BUMPUTIME(&P.p_rusage.ru_stime, TICK / 1000);
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
		c2 = &G.callout[0];
		while (c2->c_func != NULL && c2->c_dtime + t <= 0) {
			c2->c_func(c2->c_arg);
			t += c2->c_dtime;
			++c2;
		}
		c1 = &G.callout[0];
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
		++istick;
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
#if 0
	/* XXX: this shows the number of times this function has been called.
	 * It draws in the bottom-right corner of the screen.
	 */
	++*(long *)(0x4c00+0xf00-16);
#endif
	
	++walltime.tv_sec;
#if 0
	*(long *)(0x4c00+0xf00-34) = realtime.tv_nsec;
#endif
	realtime.tv_sec = walltime.tv_sec;
	realtime.tv_nsec = walltime.tv_nsec;
	
	wakeup((void *)42); /* BOGUS */
}
#endif
