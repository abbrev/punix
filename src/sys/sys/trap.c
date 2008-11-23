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
	int itimerdecr(struct itimerspec *itp, long nsec);
	
	spl5();
	/* XXX: this shows the number of times this function has been called.
	 * It draws in the bottom-right corner of the screen.
	 */
	++*(long *)(0x4c00+0xf00-8);
	
	realtime.tv_nsec += TICK;
	if (realtime.tv_nsec >= SECOND)
		realtime.tv_nsec -= TICK - TICK / HZ;
	
	if ((realtime.tv_sec % 5) == 0 && realtime.tv_nsec < TICK) {
		struct proc *p;
		int n = 0;
		for EACHPROC(p)
			if (p->p_status == P_RUNNING) ++n;
		loadav(n);
		
		batt_check();
	}
	
	/* do call-outs */
	
	if (G.callout[0].c_func == NULL)
		goto out;
	
	--G.callout[0].c_time;
	
	if (!USERMODE(ps))
		goto out;
	
	if (G.callout[0].c_time <= 0) {
		int t = 0;
		c2 = &G.callout[0];
		while (c2->c_func != NULL && c2->c_time + t <= 0) {
			c2->c_func(c2->c_arg);
			t += c2->c_time;
			++c2;
		}
		c1 = &G.callout[0];
		do
			*c1 = *c2++;
		while (c1++->c_func);
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
		int sig;
#if 0
		++P.p_ru.ru_utime;
#endif
		if (timespecisset(&P.p_itimer[ITIMER_VIRTUAL].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_VIRTUAL], TICK))
				psignal(&P, SIGVTALRM);
		
		/* preempt a running user process */
		if (runrun) {
			++istick;
			swtch();
		}
		
		while ((sig = CURSIG(&P)))
			postsig(sig);
	} else {
#if 0
		if (&P)
			++P.p_ru.ru_stime;
#endif
	}
}

#if 0
/*
 * RTC interrupt handler. This runs even when the calc is off, when hardclock()
 * does not run.
 */
STARTUP(void updrealtime())
{
	/* XXX: this shows the number of times this function has been called.
	 * It draws in the bottom-right corner of the screen.
	 */
	++*(long *)(0x4c00+0xf00-16);
	
	++realtime.tv_sec;
	realtime.tv_nsec = 0;
}
#endif
