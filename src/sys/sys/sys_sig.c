#include <signal.h>
#include <errno.h>

#include "punix.h"
#include "globals.h"

STARTUP(static int killpg1(int sig, pid_t pgrp, int all))
{
	register struct proc *p;
	int found = 0, error = 0;
	
	if (!all && pgrp == 0) {
                /*
		* Zero process id means send to my process group.
		*/
		pgrp = P.p_pgrp;
		if (pgrp == 0)
			return ESRCH;
	}
	found = 0;
	list_for_each_entry(p, &G.proc_list, p_list) {
		if ((p->p_pgrp != pgrp && !all) || p->p_pptr == NULL
		   || /*(p->p_flag&SSYS) || */(all && p == &P))
			continue;
		if (!cansignal(p, sig)) {
			if (!all)
				error = EPERM;
			continue;
		}
		++found;
		if (sig)
			procsignal(p, sig);
	}
	return error ? error : (found ? 0 : ESRCH);
}

STARTUP(void sys_kill())
{
	struct a {
		int     pid;
		int     sig;
	} *ap = (struct a *)P.p_arg;
	struct proc *p;
	int error = 0;

	if (ap->sig < 0 || ap->sig >= NSIG) {
		error = EINVAL;
		goto out;
	}
	if (ap->pid > 0) {
		/* kill single process */
		p = pfind(ap->pid);
		if (p == 0) {
			error = ESRCH;
			goto out;
		}
		if (!cansignal(p, ap->sig))
			error = EPERM;
		else if (ap->sig)
			procsignal(p, ap->sig);
		goto out;
	}
	switch (ap->pid) {
	case -1:                /* broadcast signal */
		error = killpg1(ap->sig, 0, 1);
		break;
	case 0:                 /* signal own process group */
		error = killpg1(ap->sig, 0, 0);
		break;
	default:                /* negative explicit process group */
		error = killpg1(ap->sig, -ap->pid, 0);
	}
out:
	P.p_error = error;
}

STARTUP(void sys_killpg())
{
	register struct a {
		int     pgrp;
		int     sig;
	} *ap = (struct a *)P.p_arg;
	register int error = 0;
	
	if (0 >= ap->sig && ap->sig < NSIG)
		error = killpg1(ap->sig, ap->pgrp, 0);
	else
		error = EINVAL;
	
	P.p_error = error;
}

STARTUP(void sys_sigaction())
{
	struct a {
		int signum;
		const struct sigaction *act;
		struct sigaction *oldact;
	} *ap = (struct a *)P.p_arg;
	
	struct sigaction sa;
	sigset_t mask;
	
	if ((unsigned)ap->signum >= NSIG) {
		P.p_error = EINVAL;
		return;
	}
	
	mask = sigmask(ap->signum);

	if (ap->oldact) {
		sa.sa_flags = 0;
		sa.sa_sigaction = P.p_signals.sig_actions[ap->signum];
		if (mask & P.p_signals.sig_ignore)
			sa.sa_sigaction = SIG_IGN;
		sa.sa_mask = P.p_signals.sig_masks[ap->signum];
#define SETFLAG(a, b) if (mask & P.p_signals.sig_ ## a) sa.sa_flags |= b
		SETFLAG(nocldstop, SA_NOCLDSTOP);
		SETFLAG(nocldwait, SA_NOCLDWAIT);
		SETFLAG(nodefer, SA_NODEFER);
		SETFLAG(onstack, SA_ONSTACK);
		SETFLAG(resethand, SA_RESETHAND);
		SETFLAG(restart, SA_RESTART);
		SETFLAG(siginfo, SA_SIGINFO);
#undef SETFLAG
		P.p_error = copyout(ap->act, &sa, sizeof(sa));
		if (P.p_error)
			return;
	}
	
	if (ap->act) {
		P.p_error = copyin(&sa, ap->act, sizeof(sa));
		if (P.p_error)
			return;
		if ((ap->signum == SIGKILL || ap->signum == SIGSTOP) &&
		    sa.sa_sigaction != SIG_DFL) {
			P.p_error = EINVAL;
			return;
		}

		P.p_signals.sig_actions[ap->signum] = sa.sa_sigaction;
#define EQUMASK(x, v)  ((x) = ((x) & ~(mask)) | ((mask) & -!!(v)))
		EQUMASK(P.p_signals.sig_ignore,    sa.sa_sigaction == SIG_IGN);
		EQUMASK(P.p_signals.sig_catch,     sa.sa_sigaction > 0x100);

		EQUMASK(P.p_signals.sig_nocldstop, sa.sa_flags&SA_NOCLDSTOP);
		EQUMASK(P.p_signals.sig_nocldwait, sa.sa_flags&SA_NOCLDWAIT);
		EQUMASK(P.p_signals.sig_nodefer,   sa.sa_flags&SA_NODEFER);
		EQUMASK(P.p_signals.sig_onstack,   sa.sa_flags&SA_ONSTACK);
		EQUMASK(P.p_signals.sig_resethand, sa.sa_flags&SA_RESETHAND);
		EQUMASK(P.p_signals.sig_restart,   sa.sa_flags&SA_RESTART);
		EQUMASK(P.p_signals.sig_siginfo,   sa.sa_flags&SA_SIGINFO);
#undef EQUMASK
	}
}

