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
}

STARTUP(void chk_instr())
{
}

STARTUP(void i_trapv())
{
}

STARTUP(void privilege())
{
}

STARTUP(void trace())
{
}

STARTUP(void hardclock(unsigned short ps))
{
	struct callout *c1, *c2;
	
	/* nsec cannot get up to or over 1e9 (one second); we just have to wait
	 * for the RTC to catch up to us */
	if (walltime.tv_nsec < 1000000000L - TICK) {
		walltime.tv_nsec += TICK;
		if (walltime.tv_nsec < 0)
			walltime.tv_nsec = 0;
	}
	
	/* do call-outs */
	
	if (G.callout[0].c_func == NULL)
		goto out;
	
	--G.callout[0].c_time;
	
	if (!USERMODE(ps))
		goto out;
	
	spl5();
	if (G.callout[0].c_time <= 0) {
		int t = 0;
		c2 = &G.callout[0];
		while (c2->c_func != NULL && c2->c_time + t <= 0) {
			c2->c_func(c2->c_arg);
			t += c2->c_time;
			++c2;
		}
		c1 = &G.callout[0];
		while (c2->c_func)
			*c1++ = *c2++;
	}
	
out:
	cputime += 2; /* XXX: cputime is in 15.1 fixed point */
	if (cputime >= 2 * QUANTUM) {
		++runrun;
	}
	
	if (&P) {
		if (timespecisset(&P.p_itimer[ITIMER_PROF].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_PROF], TICK))
				psignal(&P, SIGPROF);
	}
	
	if (USERMODE(ps)) {
#if 0
		++P.p_ru.ru_utime;
#endif
		if (timespecisset(&P.p_itimer[ITIMER_VIRTUAL].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_VIRTUAL], TICK))
				psignal(&P, SIGVTALRM);
		
		/* preempt a running user process */
		if (runrun)
			swtch();
		
		if (issig())
			psig();
	} else {
#if 0
		if (&P)
			++P.p_ru.ru_stime;
#endif
	}
}

/*
 * RTC interrupt handler. This runs even when the calc is off, when hardclock()
 * does not run.
 */
/* 2,000,000 ns = 0.002 seconds */
#define SKEW 2000000L

STARTUP(void updwalltime())
{
	walltime.tv_sec++;
	
	if (walltime.tv_nsec <= 0) {
		/* if the calc is turned off */
		walltime.tv_nsec = 0;
		return;
	}
	walltime.tv_nsec -= 1000000000L;
	
	/* this interrupt handler runs before the hardclock handler, so we
	 * account for this by testing against -TICK. */
	if (walltime.tv_nsec <= -TICK - SKEW)
		walltime.tv_nsec += SKEW;
	else if (walltime.tv_nsec >= -TICK + SKEW)
		walltime.tv_nsec -= SKEW;
	else
		walltime.tv_nsec = -TICK;
}
