#include <signal.h>
#include <sys/time.h>

#include "punix.h"
#include "process.h"
#include "param.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"
#include "exception.h"

/* These are all called from interrupts in entry.s */

STARTUP(static void handle_ex_bus(union exception_info *eip,
                                  const char *name,
				  int signum))
{
	kprintf("%s exception\n"
		"       function code: 0x%04x\n"
		"      access address: %p\n"
		"instruction register: 0x%04x\n"
		"     status register: 0x%04x\n"
		"     program counter: %p\n"
		"\n",
		name,
		eip->bus_error.function_code,
		eip->bus_error.access_address,
		eip->bus_error.instruction_register,
		eip->bus_error.status_register,
		eip->bus_error.program_counter
	);
	if (!USERMODE(eip->bus_error.status_register))
		panic("exception in kernel");
	psignal(current, signum);
}

STARTUP(static void handle_ex_other(union exception_info *eip,
                                    const char *name,
				    int signum))
{
	kprintf("%s exception\n"
		"     status register: 0x%04x\n"
		"       vector offset: 0x%04x\n"
		"     program counter: %p\n"
		"\n",
		name,
		eip->other.status_register,
		eip->other.vector_offset,
		eip->other.program_counter
	);
	if (!USERMODE(eip->other.status_register))
		panic("exception in kernel");
	psignal(current, signum);
}

STARTUP(void bus_error(union exception_info *eip))
{
	handle_ex_bus(eip, "bus error", SIGBUS);
}

STARTUP(void spurious(union exception_info *eip))
{
	panic("spurious exception");
}

STARTUP(void address_error(union exception_info *eip))
{
	handle_ex_bus(eip, "address error", SIGBUS);
}

STARTUP(void illegal_instr(union exception_info *eip))
{
	handle_ex_other(eip, "illegal instruction", SIGILL);
}

STARTUP(void zero_divide(union exception_info *eip))
{
	handle_ex_other(eip, "zero divide", SIGFPE);
}

STARTUP(void chk_instr(union exception_info *eip))
{
	handle_ex_other(eip, "chk instruction", SIGILL);
}

STARTUP(void i_trapv(union exception_info *eip))
{
	handle_ex_other(eip, "trapv", SIGTRAP);
}

STARTUP(void privilege(union exception_info *eip))
{
	handle_ex_other(eip, "privileged instruction", SIGILL);
}

STARTUP(void trace(union exception_info *eip))
{
	handle_ex_other(eip, "trace", SIGTRAP);
}

STARTUP(void send_sigsegv(union exception_info *eip))
{
	handle_ex_other(eip, "bad memory access", SIGSEGV);
}

STARTUP(void line1010(union exception_info *eip))
{
	handle_ex_other(eip, "line 1010 emulator", SIGILL);
}

STARTUP(void line1111(union exception_info *eip))
{
	handle_ex_other(eip, "line 1111 emulator", SIGILL);
}

#define BUMPNTIME(tv, nsec) do { \
	(tv)->tv_nsec += (nsec); \
	if ((tv)->tv_nsec >= SECOND) { \
		(tv)->tv_nsec -= SECOND; \
		(tv)->tv_sec++; \
	} \
} while (0)

/* SLEWRATE is in microseconds per tick */
#define SLEWRATE (512 / HZ) /* 512 ppm */
#define BIGADJ 1000000L /* 1 s */
#define BIGSLEWRATE (5000 / HZ) /* 5000 ppm */

STARTUP(void hardclock(unsigned short ps))
{
	int itimerdecr(struct itimerspec *itp, long nsec);
	int whereami;
	long nsec = TICK;
	
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
		timeadj -= delta;
		nsec += delta * 1000;
	}
#endif
	
	BUMPNTIME(&realtime, nsec);
	BUMPNTIME(&realtime_mono, TICK);
	BUMPNTIME(&uptime, TICK);
	G.cumulrunning += G.numrunning;
	
	sched_tick();
	
	if (current) {
		if (timespecisset(&P.p_itimer[ITIMER_PROF].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_PROF], TICK))
			psignal(current, SIGPROF);
		++current->p_cputime;
	}

	if (USERMODE(ps)) {
		++current->p_kru.kru_utime;
		if (timespecisset(&P.p_itimer[ITIMER_VIRTUAL].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_VIRTUAL], TICK))
			psignal(current, SIGVTALRM);
	} else {
		if (current)
			++current->p_kru.kru_stime;
	}
	
	// decrement the first callout's count-down timer
	if (G.callout[0].c_func)
		--G.callout[0].c_dtime;
	
	scankb();
}

void calcusage(void *unused)
{
#define CPUTICKS (HZ/2)
#define DECAY_NUM 3
#define DECAY_DEN 4
#define DECAY DECAY_NUM / DECAY_DEN
#define UNDECAY (DECAY_DEN-DECAY_NUM) / DECAY_DEN

	/* calculate percent cpu for each process */
	struct proc *p;
	int x;
	list_for_each_entry(p, &G.proc_list, p_list) {
		x = splclock();
#if 1
		p->p_pctcpu = 25600UL * p->p_cputime * UNDECAY / CPUTICKS;
		p->p_cputime = p->p_cputime * DECAY;
#else
		p->p_pctcpu = 25600UL * p->p_cputime / CPUTICKS;
		p->p_cputime = 0;
#endif
		splx(x);
	}
	timeout(calcusage, NULL, CPUTICKS);
}

void usageinit()
{
	calcusage(NULL);
}

#if 1
/*
 * RTC interrupt handler. This runs even when the calc is off, when hardclock()
 * does not run.
 */
/*
 * Note 0: since this is a single increment instruction now, it can be easily
 * moved directly to the assembly-language interrupt handler (once global
 * variables work in TIGCC)
 *
 * Note 1: to keep realtime in sync when the calc powers down, we need to keep
 * track of the delta between realtime and seconds right before power down.
 * Some code to show what I mean:
 *
 * void power_off()
 * {
 * 	// other stuff that should be done before power-down (eg, sync())
 * 	delta = realtime.tv_sec - seconds;
 * 	power_down() // the low-level power-down routine
 * 	realtime.tv_sec = seconds + delta;
 * }
 *
 * If seconds doesn't increment while it is powered off (that is, if the calc
 * is powered off for less than a second), there will be some time loss
 * (<1 second) in the realtime clock. In fact, there will be time loss whenever
 * there is less time until the next "seconds" increment after power-on than
 * before power-off. On the flip side, there will be time gain when there is
 * more time until the next increment after power-on than at power-off.
 * However, these time gains and losses are symmetrical (each should happen 50%
 * of the time, theoretically), so any drift because of this should be
 * negligible (plus we are on a graphing calculator with probably inaccurate
 * hardware clock(s) anyway).
 */
STARTUP(void updwalltime())
{
	++G.seconds;
}
#endif

extern int trygetlock(char *lock);

/*
 * return_from_int runs every time the system returns from an interrupt to the
 * base interrupt level (0). This means returning to a kernel or user process.
 */
void return_from_int(unsigned short ps, void **pc, void **usp)
{
	int sig;
	int x;
	
	/* do call-outs */
	if (trygetlock(&G.calloutlock)) {
		int t = 0;
		struct callout *c1, *c2, c;
		x = spl7();
		t = G.callout[0].c_dtime;
		while (G.callout[0].c_func && t <= 0) {
			c = G.callout[0];
			c1 = &G.callout[0];
			c2 = &G.callout[1];
			/* remove the first callout before calling it */
			do {
				*c1 = *c2++;
			} while (c1++->c_func);
			spl0();
			c.c_func(c.c_arg);
			spl7();
			t = G.callout[0].c_dtime += c.c_dtime;
		}
		spl0();
		unlock(&G.calloutlock);
	}
	
	if (!USERMODE(ps))
		return;

	/* preempt a running user process */
	if (G.need_resched)
		swtch();
	
	/*
	 * TODO:
	 * check pending signals for current process (and post one or
	 * more signals using 'pc' and 'usp' if any are pending)
	 */
	while ((sig = CURSIG(&P)))
		postsig(sig);
}
