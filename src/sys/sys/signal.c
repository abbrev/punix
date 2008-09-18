#include <errno.h>
#include <signal.h>

#include "punix.h"
#include "globals.h"

/* see 2.11BSD */
STARTUP(int issignal(struct proc *p))
{
	int sig;
	sigset_t mask;
	int prop;
	
	for (;;) {
		mask = p->p_sig & ~p->p_cursigmask;
		if (p->p_flag & SVFORK)
			mask &= ~stopsigmask; /* ??? */
		if (mask == 0)
			return 0; /* no signals to send */
		sig = ffs(mask);
		mask = sigmask(sig);
		prop = G.sigprop[sig];
		
		if (mask & p->p_sigignore && (p->p_flag & P_TRACED) == 0) {
			p->p_sig &= ~mask;
			continue;
		}
		if (p->p_flag & P_TRACED && (p->p_flag & SVFORK) == 0) {
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
			if (p->p_cursigmask & mask)
				continue;
			
			if ((p->p_flag & P_TRACED) == 0)
				continue;
			prop = G.sigprop[sig];
		}
		
		switch (P.p_sigaction[sig].sa_sigaction) {
		case SIG_DFL:
			if (p->p_pid <= 1) {
				break;
			}
			if (prop & SA_STOP) {
				if (p->p_flag & P_TRACED
				   || (p->p_pptr == &proc[1]
				   && prop & SA_TTYSTOP))
					break;
				p->p_ptracesig = sig;
				if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0)
					psignal(p->p_pptr, SIGCHLD);
				stop(p);
				swtch();
				break;
			} else if (prop & SA_IGNORE) {
				break;
			} else
				return sig;
		case SIG_IGN:
			if ((prop & SA_CONT) == 0
			   && (p->p_flag & P_TRACED) == 0)
				kprintf("issig\n");
			break;
		default:
			return sig;
		}
		p->p_sig &= ~mask;
	}
}

STARTUP(void stop(struct proc *p))
{
	p->p_status = P_STOPPED;
	p->p_flag &= ~P_WAITED;
	wakeup(p->p_pptr);
}

STARTUP(int CURSIG(struct proc *p))
{
	if (p->p_sig == 0
	    || ((p->p_flag & P_TRACED) == 0
	    && (p->p_sig & ~p->p_cursigmask) == 0))
		return 0;
	else
		return issignal(p);
}

STARTUP(int cansignal(struct proc *p, int signum))
{
	if (P.p_uid == 0                /* c effective root */
		   || P.p_ruid == p->p_ruid    /* c real = t real */
		   || P.p_uid == p->p_ruid     /* c effective = t real */
		   || P.p_ruid == p->p_uid     /* c real = t effective */
		   || P.p_uid == p->p_uid      /* c effective = t effective */
		   || (signum == SIGCONT && inferior(p)))
		return 1;
	return 0;
}

/* handle a signal */
STARTUP(void sendsig(struct proc *p, int sig, sigset_t mask))
{
	/* FIXME: write this! */
	/* the basic idea behind this is to simulate a subroutine call in
	* userland and to make it return to a stub that calls the sigreturn()
	* system call. */
	/* This is one of those problems I'll have to sleep on before I get it
	* right. */
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
}

STARTUP(void gsignal(int pgrp, int sig))
{
	struct proc *p;
	
	if (pgrp == 0)
		return;
	for EACHPROC(p)
		if (p->p_pgrp == pgrp)
			psignal(p, sig);
}

STARTUP(void postsig(int sig))
{
	sigset_t mask = sigmask(sig), returnmask;
	int (*action)();
	
	if (!P.p_fpsaved) {
		/*savefp(&P.p_fps);*/
		P.p_fpsaved = 1;
	}
	
	P.p_sig &= ~mask;
	action = P.p_sigaction[sig].sa_sigaction;
	
	if (action != SIG_DFL) {
#if 0
		if (action == SIG_IGN || (p->p_cursigmask & mask))
			panic("postsig action");
#endif
		
		spl7();
		if (P.p_psflags & SAS_OLDMASK) {
			returnmask = P.p_oldmask;
			P.p_psflags &= ~SAS_OLDMASK;
		} else
			returnmask = P.p_cursigmask;
		P.p_sigmask |= P.p_sigmask[sig] | mask;
		spl0();
		++P.p_rusage.ru_nsignals;
		sendsig(action, sig, returnmask);
		return;
	}
#if 0
	P.p_acflag |= AXSIG;
	if (G.sigprop[sig] & SA_CORE) {
		P.p_arg[0] = sig;
		if (core())
			sig |= 0200;
	}
#endif
	exit(sig);
}
