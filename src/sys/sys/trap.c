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
#include "lcd.h"

/* These are all called from interrupts in entry.s */

enum exception_type {
	EX_BUS,
	EX_OTHER
};

struct exception_signal {
	enum exception_type type;
	int signal;
	const char *name;
};

static const struct exception_signal exception_signals[] = {
	{ EX_OTHER,       0, NULL }, /* initial SP */
	{ EX_OTHER,       0, NULL }, /* initial PC */
	{ EX_BUS,    SIGBUS, "access fault" },
	{ EX_BUS,    SIGBUS, "address error" },
	{ EX_OTHER,  SIGILL, "illegal instruction" },
	{ EX_OTHER,  SIGFPE, "integer divide by zero" },
	{ EX_OTHER,  SIGFPE, "chk instruction" },
	{ EX_OTHER,  SIGFPE, "trapv instruction" },
	{ EX_OTHER,  SIGILL, "privilege violation" },
	{ EX_OTHER, SIGTRAP, "trace" },
	{ EX_OTHER,  SIGILL, "line 1010" },
	{ EX_OTHER,  SIGILL, "line 1111" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "" },
	{ EX_OTHER,       0, "spurious interrupt" }, /* spurious interrupt */
	{ EX_OTHER,       0, "" }, /* auto-int 1 */
	{ EX_OTHER,       0, "" }, /* auto-int 2 */
	{ EX_OTHER,       0, "" }, /* auto-int 3 */
	{ EX_OTHER,       0, "" }, /* auto-int 4 */
	{ EX_OTHER,       0, "" }, /* auto-int 5 */
	{ EX_OTHER,       0, "" }, /* auto-int 6 */
	{ EX_OTHER, SIGSEGV, "illegal memory access" },
	{ EX_OTHER,       0, "" }, /* trap #0 (syscall) */
	{ EX_OTHER, SIGTRAP, "trap #1" },
	{ EX_OTHER, SIGTRAP, "trap #2" },
	{ EX_OTHER, SIGTRAP, "trap #3" },
	{ EX_OTHER, SIGTRAP, "trap #4" },
	{ EX_OTHER, SIGTRAP, "trap #5" },
	{ EX_OTHER, SIGTRAP, "trap #6" },
	{ EX_OTHER, SIGTRAP, "trap #7" },
	{ EX_OTHER, SIGTRAP, "trap #8" },
	{ EX_OTHER, SIGTRAP, "trap #9" },
	{ EX_OTHER, SIGTRAP, "trap #10" },
	{ EX_OTHER, SIGTRAP, "trap #11" },
	{ EX_OTHER, SIGTRAP, "trap #12" },
	{ EX_OTHER, SIGTRAP, "trap #13" },
	{ EX_OTHER, SIGTRAP, "trap #14" },
	{ EX_OTHER, SIGTRAP, "trap #15" },
};

#define SIZEOF_SIGNALS (sizeof(exception_signals) / sizeof(exception_signals[0]))

#define ALWAYS_PRINT_EXCEPTIONS 1

STARTUP(void handle_exception(union exception_info *eip, int num))
{
	const struct exception_signal *esp;
	short sr;
	if (num < 0 || SIZEOF_SIGNALS <= num)
		panic("invalid exception number");

	esp = &exception_signals[num];
	if (esp->type == EX_BUS) {
		sr = 0; //eip->bus_error.status_register;
		if (ALWAYS_PRINT_EXCEPTIONS || !USERMODE(sr)) {
			kprintf("%s exception\n"
			        "       function code: 0x%04x\n"
				"      access address: %p\n"
				"instruction register: 0x%04x\n"
				"     status register: 0x%04x\n"
				"     program counter: %p\n"
				"\n",
			        esp->name,
				eip->bus_error.function_code,
				eip->bus_error.access_address,
				eip->bus_error.instruction_register,
				eip->bus_error.status_register,
				eip->bus_error.program_counter
			);
		}
	} else {
		sr = eip->other.status_register;
		if (ALWAYS_PRINT_EXCEPTIONS || !USERMODE(sr)) {
			kprintf("%s exception\n"
			        "     status register: 0x%04x\n"
				"     program counter: %p\n"
				"       vector offset: 0x%04x\n"
				"             address: %p\n"
				"\n",
			        esp->name,
				eip->other.status_register,
				eip->other.program_counter,
				eip->other.vector_offset,
				eip->other.address
			);
		}
	}
	if (!USERMODE(sr))
		panic("exception in kernel");
	procsignal(current, esp->signal);
}

#define BUMPNTIME(tv, nsec) do { \
	(tv)->tv_nsec += (nsec); \
	if ((tv)->tv_nsec >= SECOND) { \
		(tv)->tv_nsec -= SECOND; \
		(tv)->tv_sec++; \
	} \
} while (0)

// allow both positive and negative amounts
#define ADDNTIME(tv, nsec) do { \
	(tv)->tv_nsec += (nsec); \
	while ((tv)->tv_nsec >= SECOND) { \
		(tv)->tv_nsec -= SECOND; \
		(tv)->tv_sec++; \
	} \
	while ((tv)->tv_nsec < 0) { \
		(tv)->tv_nsec += SECOND; \
		(tv)->tv_sec--; \
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
	
	//splclock(); // this is pointless
	
	whereami = G.whereami;
	++G.ticks;
	++G.monoticks;

	extern void updategray();
	updategray();

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
		BUMPNTIME(&realtime_offset, nsec);
	}
#endif
	
	BUMPNTIME(&uptime, TICK);
	G.cumulrunning += G.numrunning;
	
	sched_tick();
	
	if (current) {
		if (timespecisset(&P.p_itimer[ITIMER_PROF].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_PROF], TICK))
			procsignal(current, SIGPROF);
		++current->p_cputime;
	} else {
		++*(long *)(0x4c00+0xf00-30*2+4);
	}

	if (USERMODE(ps)) {
		++current->p_kru.kru_utime;
		if (timespecisset(&P.p_itimer[ITIMER_VIRTUAL].it_value) &&
		    !itimerdecr(&P.p_itimer[ITIMER_VIRTUAL], TICK))
			procsignal(current, SIGVTALRM);
	} else {
		if (current)
			++current->p_kru.kru_stime;
	}
	
	// decrement the first callout's count-down timer
	if (G.callout[0].c_func)
		--G.callout[0].c_dtime;
	
	//scankb();
}

/*
 * XXX: Currently this calculates an exponential moving average of a process's
 * cpu usage. It might be more desirable to calculate the linear moving average
 * instead, as follows:
 * * keep track of a process's cpu usage for the last several time units in a
 *   small array (in struct proc). a char array could be used.
 * * also maintain the current sum of the array
 * * each time this routine runs, subtract the "head" value from the sum and
 *   add the p_cputime to the sum
 * * write the p_cputime to the head of the array and clear p_cputime
 * * this will maintain the numerator of the average with O(1) complexity
 * * divide the numerator (sum) by a fixed denominator, ideally a power of 2
 */
void calcusage(const void *unused)
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
 */
STARTUP(void updwalltime())
{
	++G.seconds;
	G.monoticks = 0;
}
#endif

/*
 * return_from_int runs every time the system returns from an interrupt to the
 * base interrupt level (0). This means returning to a kernel or user process.
 */
void return_from_int(unsigned short ps, void **pc, void **usp)
{
	int sig;
	int x;
	
	/* do call-outs */
	if (mask(&G.calloutlock) == 0) {
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
			t = G.callout[0].c_dtime += c.c_dtime;
			spl0();
			c.c_func(c.c_arg);
			spl7();
		}
		spl0();
	}
	unmask(&G.calloutlock);

	if (!USERMODE(ps))
		return;

	G.whereami = WHEREAMI_USER;

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

#define LCD_ROW_SYNC (*(volatile char *)0x60001c)
#define LCD_CONTRAST (*(volatile char *)0x60001d)
#define LCD_CONTROL  (*(volatile char *)0x70001d)

void cpupoweroff()
{
	G.onkey = 0;
	G.powerstate = 1;
	
	LCD_ROW_SYNC = 0b00111100; // turn off row sync
	LCD_CONTRAST |= (1<<4);   // disable screen (hw1)
	LCD_CONTROL &= ~(1<<1);  // shut down LCD (hw2)
	
	while (G.monoticks)
		;
	
	long s = seconds; // start time of poweroff state

	while (!G.onkey)
		cpuidle(INT_3|INT_4);
	
	int splclock();
	if (timeadj) {
		/*
		 * Continue the time adjustment made by adjtime() by stepping
		 * realtime_offset by 5000ppm of the time the calculator was
		 * turned off. This is equivalent to BIGSLEWRATE, defined
		 * above.
		 *
		 * Untested and probably buggy!
		 */
		long sdelta = (s - seconds);
		long t;
		if (timeadj > 0) {
			if (sdelta >= timeadj / 5000) {
				t = timeadj;
			} else {
				t = sdelta * 5000;
			}
		} else /*if (timeadj < 0)*/ {
			if (-sdelta <= timeadj / 5000) {
				t = timeadj;
			} else {
				t = -sdelta * 5000;
			}
		}
		ADDNTIME(&G.realtime_offset, t * 1000);
		timeadj -= t;
	}
	splx(x);
	
	LCD_CONTROL |= (1<<1);
	LCD_CONTRAST &= ~(1<<4);
	LCD_ROW_SYNC = 0b00100001;
	lcd_reset_contrast(); // contrast was reset when 0x60001d (LCD_CONTRAST) was modified

	G.powerstate = 0;
}

