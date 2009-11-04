#include <errno.h>
#include <signal.h>
#include <string.h>

#include "param.h"
#include "punix.h"
#include "signal.h"
#include "globals.h"
#include "wait.h"

/* FIXME! make sure these signals match those in <signal.h> */
static const char sigprop[NSIG + 1] = {
	0,                      /* unused */
	SA_KILL,                /* SIGHUP */
	SA_KILL,                /* SIGINT */
	SA_KILL|SA_CORE,        /* SIGQUIT */
	SA_KILL|SA_CORE,        /* SIGILL */
	SA_KILL|SA_CORE,        /* SIGTRAP */
	SA_KILL|SA_CORE,        /* SIGABRT */
	SA_KILL|SA_CORE,        /* SIGEMT */
	SA_KILL|SA_CORE,        /* SIGFPE */
	SA_KILL,                /* SIGKILL */
	SA_KILL|SA_CORE,        /* SIGBUS */
	SA_KILL|SA_CORE,        /* SIGSEGV */
	SA_KILL|SA_CORE,        /* SIGSYS */
	SA_KILL,                /* SIGPIPE */
	SA_KILL,                /* SIGALRM */
	SA_KILL,                /* SIGTERM */
	SA_IGNORE,              /* SIGURG */
	SA_STOP,                /* SIGSTOP */
	SA_STOP|SA_TTYSTOP,     /* SIGTSTP */
	SA_IGNORE|SA_CONT,      /* SIGCONT */
	SA_IGNORE,              /* SIGCHLD */
	SA_STOP|SA_TTYSTOP,     /* SIGTTIN */
	SA_STOP|SA_TTYSTOP,     /* SIGTTOU */
	SA_IGNORE,              /* SIGIO */
	SA_KILL,                /* SIGXCPU */
	SA_KILL,                /* SIGXFSZ */
	SA_KILL,                /* SIGVTALRM */
	SA_KILL,                /* SIGPROF */
	SA_IGNORE,              /* SIGWINCH  */
	SA_IGNORE,              /* SIGINFO */
	SA_KILL,                /* SIGUSR1 */
	SA_KILL,                /* SIGUSR2 */
};

STARTUP(void stop(struct proc *p))
{
	sched_stop(p);
	p->p_flag &= ~P_WAITED;
	wakeup(p->p_pptr);
}

STARTUP(int CURSIG(struct proc *p))
{
	if (p->p_sig == 0
	    || (!(p->p_flag & P_TRACED) && !(p->p_sig & ~p->p_sigmask))) {
		return 0;
	} else {
		return issignal(p);
	}
}

STARTUP(int cansignal(struct proc *p, int signum))
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

STARTUP(void psignal(struct proc *p, int sig))
{
#if 0 /* not ready yet (some of these fields don't exist in struct proc) */
	if ((unsigned)sig >= NSIG)
	return;
	if (sig)
	p->p_sig |= 1 << (sig-1);
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
		if (p->p_sigignore & mask)
			return;
		if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}
	
	if (p->p_nice > NZERO && action == SIG_DFL && (prop & SA_KILL)
	    && (p->p_flag & P_TRACED) == 0)
		p->p_nice = NZERO;
	
	if (prop & SA_CONT)
		p->p_sig &= ~STOPSIGMASK;
	
	if (prop & SA_STOP) {
		if (prop & SA_TTYSTOP && (p->p_pptr == G.initproc)
		    && action == SIG_DFL)
			return;
		p->p_sig &= ~CONTSIGMASK;
	}
	p->p_sig |= mask;
	
	if (action == SIG_HOLD && (!(prop & SA_CONT) || p->p_status != P_STOPPED))
		return;
	s = spl7();
	switch (p->p_status) {
	case P_SLEEPING:
		if (!(p->p_flag & P_SINTR))
			goto out;
		
		if (p->p_flag & P_TRACED)
			goto run;
		
		if ((prop & SA_CONT) && action == SIG_DFL) {
			p->p_sig &= ~mask;
			goto out;
		}
		
		if (prop & SA_STOP) {
			if (action != SIG_DFL)
				goto run;
			if (p->p_flag & P_VFORK)
				goto out;
			p->p_sig &= ~mask;
			p->p_ptracesig = sig;
			if (!(p->p_pptr->p_flag & P_NOCLDSTOP))
				psignal(p->p_pptr, SIGCHLD);
			stop(p);
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
				p->p_sig &= ~mask;
			if (action == SIG_CATCH || p->p_waitchan == 0)
				goto run;
			/* p->p_status = P_SLEEPING; */
			sched_sleep(p);
			goto out;
		}
		
		if (prop & SA_STOP) {
			p->p_sig &= ~mask;
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
	//kprintf("psignal(): gonna run setrun()\n");
	setrun(p);
out:
	splx(s);
}

STARTUP(void gsignal(int pgrp, int sig))
{
	struct proc *p;
	
	if (pgrp == 0)
		return;
	list_for_each_entry(p, &G.proc_list, p_list)
		if (p->p_pgrp == pgrp)
			psignal(p, sig);
}

/* see 2.11BSD */
STARTUP(int issignal(struct proc *p))
{
	int sig;
	sigset_t mask;
	int prop;
	
	for (;;) {
		mask = p->p_sig & ~p->p_sigmask;
		if (p->p_flag & P_VFORK)
			mask &= ~STOPSIGMASK;
		if (mask == 0)
			return 0; /* no signals to send */
		sig = ffsl(mask);
		mask = sigmask(sig);
		prop = sigprop[sig];
		
		if (mask & p->p_sigignore && (p->p_flag & P_TRACED) == 0) {
			p->p_sig &= ~mask;
			continue;
		}
#if 0
		if (p->p_flag & P_TRACED && (p->p_flag & P_VFORK) == 0) {
			p->p_sig &= ~mask;
			p->p_ptracesig = sig;
			psignal(p->p_pptr, SIGCHLD);
			do {
				stop(p);
				swtch();
			} while (!procxmt() && p->p_flag & P_TRACED);
			
			sig = p->p_ptracesig;
			if (sig == 0)
				continue;
			
			mask = sigmask(sig);
			p->p_sig |= mask;
			if (p->p_sigmask & mask)
				continue;
			
			if ((p->p_flag & P_TRACED) == 0)
				continue;
			prop = sigprop[sig];
		}
#endif
		
		switch ((intptr_t)P.p_signal[sig]) {
		case (intptr_t)SIG_DFL:
#if 0 /* for testing (but why would a sane init process leave any of its signal handlers at their default?) */
			if (p->p_pid <= 1) {
				break;
			}
#endif
			if (prop & SA_STOP) {
				if ((p->p_pptr==G.initproc && prop&SA_TTYSTOP)
				   || (p->p_flag&P_TRACED))
					break;
				p->p_ptracesig = sig;
				if (!(p->p_pptr->p_flag & P_NOCLDSTOP))
					psignal(p->p_pptr, SIGCHLD);
				stop(p);
				swtch();
				break;
			} else if (prop & SA_IGNORE) {
				break;
			} else
				return sig;
		case (intptr_t)SIG_IGN:
			if ((prop & SA_CONT) == 0 && (p->p_flag & P_TRACED) == 0)
				kprintf("issig\n");
			break;
		default:
			return sig;
		}
		p->p_sig &= ~mask;
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
	
	P.p_sig &= ~mask;
	action = P.p_signal[sig];
	
	//kprintf("postsig\n");
	if (action != SIG_DFL) {
#if 0
		if (action == SIG_IGN || (p->p_sigmask & mask))
			panic("postsig action");
#endif
		
		spl7();
		if (P.p_psflags & SAS_OLDMASK) {
			returnmask = P.p_oldmask;
			P.p_psflags &= ~SAS_OLDMASK;
		} else
			returnmask = P.p_sigmask;
		P.p_sigmask |= P.p_sigmasks[sig] | mask;
		spl0();
		++P.p_rusage.ru_nsignals;
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
