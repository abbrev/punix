#include <errno.h>
#include <signal.h>
#include <string.h>

#include "param.h"
#include "punix.h"
#include "globals.h"
#include "wait.h"

#define SAS_OLDMASK  0x01 /* need to restore mask before pause */
#define SAS_ALTSTACK 0x02 /* have alternate signal stack */

#define SIG_CATCH ((sighandler_t)3)

#define CONTSIGMASK     (sigmask(SIGCONT))
#define STOPSIGMASK     (sigmask(SIGSTOP) | sigmask(SIGTSTP) | \
                         sigmask(SIGTTIN) | sigmask(SIGTTOU))
#define SIGCANTMASK     (sigmask(SIGKILL) | sigmask(SIGSTOP))

/*
 * Signal properties and actions.
 * The array below categorizes the signals and their default actions
 * according to the following properties:
 */
#define SA_KILL    0x01 /* terminates process by default */
#define SA_CORE    0x02 /* ditto and coredumps */
#define SA_STOP    0x04 /* suspend process */
#define SA_TTYSTOP 0x08 /* ditto, from tty */
#define SA_IGNORE  0x10 /* ignore by default */
#define SA_CONT    0x20 /* continue if suspended */

#define T (SA_KILL)
#define A (SA_KILL|SA_CORE)
#define I (SA_IGNORE)
#define S (SA_STOP)
#define C (SA_CONT|SA_IGNORE)

static const char sigprop[NSIG + 1] = {
	[0]         = I, /* null signal */
	[SIGABRT]   = A,
	[SIGALRM]   = T,
	[SIGBUS]    = A,
	[SIGCHLD]   = I,
	[SIGCONT]   = C,
	[SIGFPE]    = A,
	[SIGHUP]    = T,
	[SIGILL]    = A,
	[SIGINT]    = T,
	[SIGKILL]   = T,
	[SIGPIPE]   = T,
	[SIGQUIT]   = A,
	[SIGSEGV]   = A,
	[SIGSTOP]   = S,
	[SIGTERM]   = T,
	[SIGTSTP]   = S|SA_TTYSTOP,
	[SIGTTIN]   = S|SA_TTYSTOP,
	[SIGTTOU]   = S|SA_TTYSTOP,
	[SIGUSR1]   = T,
	[SIGUSR2]   = T,
	[SIGPOLL]   = T,
	[SIGPROF]   = T,
	[SIGSYS]    = A,
	[SIGTRAP]   = A,
	[SIGURG]    = I,
	[SIGVTALRM] = T,
	[SIGXCPU]   = A,
	[SIGXFSZ]   = A,

	/* following are not defined by POSIX */
	[SIGEMT]    = A,
	[SIGWINCH]  = I,
	[SIGINFO]   = I,
};

#undef T
#undef A
#undef I
#undef S
#undef C

STARTUP(static void stop(struct proc *p, int sig))
{
	sched_stop(p);
	p->p_flag &= ~P_WAITED;
	p->p_waitstat = W_STOPCODE(sig);
#if 0
	if (p->p_pptr) {
		procsignal(p->p_pptr, SIGCHLD);
		//wakeup(p->p_pptr);
	}
#endif
}

STARTUP(int CURSIG(struct proc *p))
{
	if (p->p_signals.sig_pending == 0 ||
	    (!(p->p_flag & P_TRACED) &&
	     !(p->p_signals.sig_pending & ~p->p_sigmask))) {
		return 0;
	} else {
		return issignal(p);
	}
}

STARTUP(int cansignal(struct proc const *p, int signum))
{
	if (P.p_euid == 0 ||           /* c effective root */
	    P.p_ruid == p->p_ruid ||   /* c real = t real */
	    P.p_euid == p->p_ruid ||   /* c effective = t real */
	    P.p_ruid == p->p_euid ||   /* c real = t effective */
	    P.p_euid == p->p_euid ||   /* c effective = t effective */
	    (signum == SIGCONT && inferior(p)))
		return 1;
	return 0;
}

/* handle a signal */
STARTUP(void sendsig(struct proc *p, int sig, sigset_t returnmask))
{
	/* FIXME: write this! */
	/* the basic idea behind this is to simulate a subroutine call in
	* userland and to make it return to a stub that calls the sigreturn()
	* system call. */
	/* This is one of those problems I'll have to sleep on before I get it
	* right. */
	P.p_sigmask = returnmask;
}

STARTUP(void procsignal(struct proc *p, int sig))
{
#if 0 /* not ready yet (some of these fields don't exist in struct proc) */
	if ((unsigned)sig >= NSIG)
	return;
	if (sig)
	p->p_signals.sig_pending |= 1 << (sig-1);
	if (p->p_pri > PUSER)
	p->p_pri = PUSER;
	if (p->p_stat == P_SLEEPING && p->p_pri > PZERO)
	setrun(p);
#endif
	int s;
	void (*action)();
	int prop;
	sigset_t mask;
	
	mask = sigmask(sig);
	prop = sigprop[sig];
	
	if (p->p_flag & P_TRACED)
		action = SIG_DFL;
	else {
		if (p->p_sigignore & mask) {
			return;
		}
		if (p->p_sigmask & mask) {
			action = SIG_HOLD;
		} else if (p->p_sigcatch & mask) {
			action = SIG_CATCH;
		} else {
			action = SIG_DFL;
		}
	}
	
#if 0
	if (p->p_nice > NZERO && action == SIG_DFL && (prop & SA_KILL)
	    && (p->p_flag & P_TRACED) == 0)
		p->p_nice = NZERO;
#endif
	
	if (prop & SA_CONT)
		p->p_signals.sig_pending &= ~STOPSIGMASK;
	
	if (prop & SA_STOP) {
		if (prop & SA_TTYSTOP && (p->p_pptr == G.initproc)
		    && action == SIG_DFL)
			return;
		p->p_signals.sig_pending &= ~CONTSIGMASK;
	}
	p->p_signals.sig_pending |= mask;
	
	if (action == SIG_HOLD && (!(prop & SA_CONT) || p->p_state != P_STOPPED))
		return;
	s = spl7();
	switch (p->p_state) {
	case P_SLEEPING:
		if (!(p->p_flag & P_SINTR))
			goto out;
		
		if (p->p_flag & P_TRACED)
			goto run;
		
		if ((prop & SA_CONT) && action == SIG_DFL) {
			p->p_signals.sig_pending &= ~mask;
			goto out;
		}
		
		if (prop & SA_STOP) {
			if (action != SIG_DFL)
				goto run;
			if (p->p_flag & P_VFORK)
				goto out;
			p->p_signals.sig_pending &= ~mask;
			p->p_ptracesig = sig;
			if (!(p->p_pptr->p_flag & P_NOCLDSTOP))
				procsignal(p->p_pptr, SIGCHLD);
			stop(p, sig);
			goto out;
		}
		goto run;
	case P_STOPPED:
		if (p->p_flag & P_TRACED)
			goto out;
		if (sig == SIGKILL)
			goto run;
		if (prop & SA_CONT) {
			if (action == SIG_DFL)
				p->p_signals.sig_pending &= ~mask;
			if (action == SIG_CATCH || p->p_waitchan == 0)
				goto run;
			/* p->p_state = P_SLEEPING; */
			sched_sleep(p);
			goto out;
		}
		
		if (prop & SA_STOP) {
			p->p_signals.sig_pending &= ~mask;
			goto out;
		}
		
		if (p->p_waitchan && (p->p_flag & P_SINTR))
			unsleep(p);
		goto out;
	default:
		goto out;
	}
run:
	//if (p->p_pri > PUSER)
		//p->p_pri = PUSER;
	//kprintf("procsignal(): gonna run setrun()\n");
	TRACE();
	setrun(p);
out:
	splx(s);
}

STARTUP(void gsignal(int pgrp, int sig))
{
	struct proc *p;
	int x;
	
	if (pgrp == 0)
		return;
	mask(&G.calloutlock);
	list_for_each_entry(p, &G.proc_list, p_list)
		if (p->p_pgrp == pgrp)
			procsignal(p, sig);
	unmask(&G.calloutlock);
}

/* see 2.11BSD */
STARTUP(int issignal(struct proc *p))
{
	int sig;
	sigset_t mask;
	int prop;
	
	for (;;) {
		mask = p->p_signals.sig_pending & ~p->p_sigmask;
		if (p->p_flag & P_VFORK)
			mask &= ~STOPSIGMASK;
		if (mask == 0)
			return 0; /* no signals to send */
		sig = ffsl(mask);
		mask = sigmask(sig);
		prop = sigprop[sig];
		
		if (mask & p->p_sigignore && (p->p_flag & P_TRACED) == 0) {
			p->p_signals.sig_pending &= ~mask;
			continue;
		}
#if 0
		if (p->p_flag & P_TRACED && (p->p_flag & P_VFORK) == 0) {
			p->p_signals.sig_pending &= ~mask;
			p->p_ptracesig = sig;
			procsignal(p->p_pptr, SIGCHLD);
			do {
				stop(p, sig);
				swtch();
			} while (!procxmt() && p->p_flag & P_TRACED);
			
			sig = p->p_ptracesig;
			if (sig == 0)
				continue;
			
			mask = sigmask(sig);
			p->p_signals.sig_pending |= mask;
			if (p->p_sigmask & mask)
				continue;
			
			if ((p->p_flag & P_TRACED) == 0)
				continue;
			prop = sigprop[sig];
		}
#endif
		
		if (P.p_signals.sig_catch & mask) {
			return sig;
		} else if (P.p_signals.sig_ignore & mask) {
			/* this signal is ignored */
#if 0
			if ((prop & SA_CONT) == 0 && (p->p_flag & P_TRACED) == 0)
				kprintf("issig\n");
#endif
		} else {
			/* default signal handler */
#if 0 /* for testing (but why would a sane init process leave any of its signal handlers at their default?) */
			if (p->p_pid <= 1) {
				break;
			}
#endif
			if (prop & SA_STOP) {
				if ((p->p_pptr==G.initproc && prop&SA_TTYSTOP)
				   || (p->p_flag&P_TRACED)) {
					/* nothing */
				} else {
					p->p_ptracesig = sig;
					if (!(p->p_pptr->p_flag & P_NOCLDSTOP))
						procsignal(p->p_pptr, SIGCHLD);
					stop(p, sig);
					swtch();
				}
			} else if (prop & SA_IGNORE) {
				/* ignore this signal by default */
			} else {
				return sig;
			}
		}
		p->p_signals.sig_pending &= ~mask;
	}
}

STARTUP(void postsig(int sig))
{
	sigset_t mask = sigmask(sig), returnmask;
	void (*action)();
	
	if (!P.p_fpsaved) {
		/*savefp(&P.p_fps);*/
		P.p_fpsaved = 1;
	}
	
	P.p_signals.sig_pending &= ~mask;
	action = P.p_signal[sig];
	
	//kprintf("postsig\n");
	if (action != SIG_DFL) {
#if 0
		if (action == SIG_IGN || (p->p_sigmask & mask))
			panic("postsig action");
#endif
		
		int x = spl7();
#if 0
		if (P.p_psflags & SAS_OLDMASK) {
			returnmask = P.p_oldmask;
			P.p_psflags &= ~SAS_OLDMASK;
		} else
#endif
			returnmask = P.p_sigmask;
		P.p_sigmask |= P.p_sigmasks[sig] | mask;
		splx(x);
		++P.p_kru.kru_nsignals;
		sendsig(action, sig, returnmask);
		return;
	}
#if 0
	P.p_acflag |= AXSIG;
	if (sigprop[sig] & SA_CORE) {
		P.p_arg[0] = sig;
		if (core())
			sig |= 0200;
	}
#endif
	doexit(W_TERMCODE(sig));
}
