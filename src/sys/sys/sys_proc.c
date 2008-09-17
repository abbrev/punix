/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
 * 
 * $Id: sys_proc.c,v 1.14 2008/04/18 02:47:22 fredfoobar Exp $
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "setjmp.h"
#include "proc.h"
#include "punix.h"
#include "process.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* process manipulation system calls */

STARTUP(void sys_sigreturn())
{
	/* FIXME: write this! */
}

/* XXX: user functions for testing */
extern ssize_t write(int fd, const void *buf, size_t count);
extern void _exit(int status);

extern void userstart();

STARTUP(static void println(char *s))
{
	write(2, s, strlen(s));
	write(2, "\n", 1);
}

STARTUP(int usermain(int argc, char *argv[], char *envp[]))
{
	println("This is a user program.");
	
	println("\narguments:");
	for (; *argv; ++argv)
		println(*argv);
	
	println("\nenvironment:");
	for (; *envp; ++envp)
		println(*envp);
	
	return 0;
}
/* XXX */

STARTUP(static void copyenv(char ***vp, char **sp, char **sv))
{
	char *cp;
	char **v = *vp;
	char *s = *sp;
	
	for (; (cp = *sv); ++sv) {
		*v++ = s; /* set this vector for this string */
		while ((*s++ = *cp++) != '\0') /* copy the string */
			;
	}
	*v++ = NULL;
	*vp = v;
	*sp = s;
}

/* the following are inherited by the child from the parent (this list comes from execve(2) man page in FreeBSD 6.2):
	process ID           see getpid(2)
	parent process ID    see getppid(2)
	process group ID     see getpgrp(2)
	access groups        see getgroups(2)
	working directory    see chdir(2)
	root directory       see chroot(2)
	control terminal     see termios(4)
	resource usages      see getrusage(2)
	interval timers      see getitimer(2)
	resource limits      see getrlimit(2)
	file mode mask       see umask(2)
	signal mask          see sigvec(2), sigsetmask(2)
Essentially, don't touch those (except for things like signals set to be caught
are set to default in the new image).
*/
STARTUP(void sys_execve())
{
	struct a {
		const char *pathname;
		char **argp;
		char **envp;
	} *uap = (struct a *)P.p_arg;
	
	const char *pathname = uap->pathname;
	char **argp = uap->argp;
	char **envp = uap->envp;
	
	/*
	 * XXX: this version does not load the binary from a file, allocate
	 * memory, or any of that fun stuff (as these capabilities have not
	 * been implemented in Punix yet).
	 */
	
	
	void *text = userstart; /* XXX */
	char *data = NULL;
	char *stack = NULL;
	
	long size = 0;
	char **ap;
	char *s, **v;
	int argc, envc;
	
	/* set up the user's stack (argc, argv, and env) */
	
	/* calculate the size of the arguments and environment + the size of
	 * their vectors */
	
	for (ap = argp; *ap; ++ap)
		size += strlen(*ap) + 1;
	argc = ap - argp; /* number of arg vectors */
	
	for (ap = envp; *ap; ++ap)
		size += strlen(*ap) + 1;
	envc = ap - envp; /* number of env vectors */
	
	/* XXX: allocate mem for the stack */
	stack = &G.ustack[USTACKSIZE];
	
	size = (size + 1) & ~1; /* size must be even */
	s = stack - size; /* start of strings */
	v = (char **)s - (argc + envc + 2); /* start of vectors */
	
	stack = (char *)v;
	*--(int *)stack = argc;
	
	/* copy arguments */
	copyenv(&v, &s, argp);
	copyenv(&v, &s, envp);
	
#if 0
	ustack -= strlen(argp[0]) + 1;
	argv0 = ustack;
	strcpy(argv0, argp[0]);
	*--ustack = NULL; /* env[nenv] */
	*--ustack = NULL; /* argv[narg] */
	*--ustack = argv0; /* argv[0] */
	*--(int *)ustack = 1; /* argc */
#endif
	
	if (P.p_pptr && P.p_pptr->p_status == P_VFORKING)
		P.p_pptr->p_status = P_RUNNING;
	else {
		/* free our resources */
	}
	
	/* finally, set up the context to return to the new process image */
	if (setjmp(P.p_ssav))
		return;
	
	/* go to the new user context */
	P.p_ssav->usp = stack;
	P.p_tfp->pc = text;
	longjmp(P.p_ssav, 1);
	
	return;
	
	/*
	 * open binary file (filename is given in the file if it's a script)
	 * allocate memory for user stack
	 * allocate memory for user data
	 * allocate memory for user text
	 * load text and data from binary file
	 * copy environment from old image to new image (special case for argv
	 *   if file is a script)
	 * close files set close-on-exec
	 * reset signal handlers
	 * set PC and SP in proc structure
	 * free resources used by the old image if it is not a vfork child
	 * wake up the parent if it is vforking
	 * return as normal
	 * 
	 * on error (error code is set prior to jumping to one of these points):
	 * close binary file
	 * free memory for user text
	 * free memory for user data
	 * free memory for user stack
	 * return
	 */
}

/* XXX: fix this to conform to the new vfork() method */
STARTUP(void sys_exit())
{
	struct a {
		int status;
	} *ap = (struct a *)P.p_arg;
	
	if (P.p_pid == 1)
		panic("process 1 exited");
	
	CURRENT->p_waitstat = ap->status << 8;
	/* clean up resources used by proc */
	/*
	 * we probably don't need to free memory allocations, as we probably
	 * don't need to increment ref counts to mem allocs upon vfork
	 * This is because a vfork parent is blocked until the child exits or
	 * execs anyway
	 */
	/* close all fd's */
	/* any more we have to do? */
	/* FIXME: write this */
	
	/* FIXME: add signals to appropriate global header file and fix this */
	psignal(P.p_pptr, SIGCHLD);
	
	P.p_status = P_ZOMBIE;
	/* if parent is blocked by us with vfork, we need to wake it up */
	
	swtch();
}

STARTUP(void sys_fork())
{
	P.p_error = ENOSYS;
}

STARTUP(void sys_vfork())
{
	struct proc *cp = NULL;
	pid_t pid;
	void *sp = NULL;
	void setup_env(jmp_buf env, struct trapframe *tfp, long *sp);
	
	goto nomem; /* XXX: remove this once vfork is completely written */
	
	/* spl7(); */
	
	cp = palloc();
	if (!cp)
		goto nomem;
	
	*cp = P; /* copy our own process structure to the child */
	
	pid = pidalloc();
	P.p_retval = pid;
	cp->p_pid = pid;
	
	/* allocate a kernel stack for the child */
	/* sp = kstackalloc(); */
	if (!sp)
		goto stackp;
	
	/* At this point there's no turning back! */
	
	cp->p_stack = sp;
	cp->p_stacksize = 0; /* XXX */
	cp->p_status = P_RUNNING;
	
	/* use the new kernel stack but the same user stack */
	setup_env(cp->p_ssav, P.p_tfp, sp);
	
	P.p_status = P_VFORKING;
	swtch();
	
	return;
stackp:
	pfree(cp);
nomem:
	P.p_error = ENOMEM;
}

STARTUP(static void dowait4(
		pid_t pid,
		int *status,
		int options,
		struct rusage *rusage))
{
	struct proc *p;
	int nfound = 0;
	int s;
	struct rusage r;
	int error;
	
loop:
	for EACHPROC(p)
	if (p->p_pptr == &P) {
		if (pid > 0) {
			if (p->p_pid != pid) continue;
		} else if (pid == 0) {
			if (p->p_pgrp != P.p_pgrp) continue;
		} else if (pid < (pid_t)-1) {
			if (p->p_pgrp != -pid) continue;
		}
		++nfound;
		if ((P.p_sigaction[SIGCHLD].sa_flags & SA_NOCLDWAIT)
		    || P.p_sigaction[SIGCHLD].sa_handler == SIG_IGN) {
			continue;
		}
		if ((options & WCONTINUED) && p->p_status == P_RUNNING && (p->p_flag & P_WAITED)) {
			/* return with this proc */
			p->p_status &= ~P_WAITED;
			goto found;
		}
		if ((options & WUNTRACED) && p->p_status == P_STOPPED && !(p->p_flag & P_WAITED)) {
			/* return with this proc */
			p->p_status |= P_WAITED;
			goto found;
		}
		if (p->p_status == P_ZOMBIE) {
			/* FIXME: free this proc for use by new processes */
			goto found;
		}
	}
	
	if (!nfound) {
		P.p_error = ECHILD;
		return;
	}
	if (options & WNOHANG) {
		return;
	}
	error = tsleep(&P, PWAIT|PCATCH, 0);
	if (!error) goto loop;
	return;
	
found:
	P.p_retval = p->p_pid;
	if (status) {
		if (copyout(status, &p->p_waitstat, sizeof(s)))
			P.p_error = EFAULT;
	}
	
	if (rusage) {
		/* fill in rusage */
		if (copyout(rusage, &p->p_rusage, sizeof(r)))
			P.p_error = EFAULT;
	}
	
	if (p->p_status == P_ZOMBIE) {
		/* FIXME: free this proc */
	}
}

/* first arg is the same as wait */
struct wait3a {
	int *status;
	int options;
	struct rusage *rusage;
};

/* first 3 args are the same as waitpid */
struct wait4a {
	pid_t pid;
	int *status;
	int options;
	struct rusage *rusage;
};

STARTUP(void sys_wait())
{
	struct wait3a *ap = (struct wait3a *)P.p_arg;
	dowait4(-1, ap->status, 0, NULL);
}

STARTUP(void sys_waitpid())
{
	struct wait4a *ap = (struct wait4a *)P.p_arg;
	dowait4(ap->pid, ap->status, ap->options, NULL);
}

/*
 * The following two system calls are XSI extensions and are not necessary for
 * POSIX comformance.
 */

#if 0
STARTUP(void sys_wait3())
{
	struct wait3a *ap = (struct wait3a *)P.p_arg;
	dowait4(-1, ap->status, ap->options, ap->rusage);
}

STARTUP(void sys_wait4())
{
	struct wait4a *ap = (struct wait4a *)P.p_arg;
	dowait4(ap->pid, ap->status, ap->options, ap->rusage);
}
#endif

STARTUP(void sys_getpid())
{
	P.p_retval = P.p_pid;
}

STARTUP(void sys_getppid())
{
	P.p_retval = P.p_pptr->p_pid;
}

STARTUP(static void donice(struct proc *p, int n))
{

        if (P.p_uid && P.p_ruid &&
            P.p_uid != p->p_uid && P.p_ruid != p->p_uid) {
                P.p_error = EPERM;
                return;
        }
        if (n > PRIO_MAX)
                n = PRIO_MAX;
        if (n < PRIO_MIN)
                n = PRIO_MIN;
        if (n < p->p_nice && !suser()) {
                P.p_error = EACCES;
                return;
        }
        p->p_nice = n;
}

STARTUP(void sys_nice())
{
	struct a {
		int inc;
	} *ap = (struct a *)P.p_arg;
	
	donice(&P, P.p_nice + ap->inc);
	P.p_retval = P.p_nice - NZERO;
}

STARTUP(void sys_getpriority())
{
	struct a {
		int which;
		id_t who;
	} *ap = (struct a *)P.p_arg;
	struct proc *p;
	pid_t pgrp;
	uid_t uid;
	int nice = PRIO_MAX + 1;
	
	switch (ap->which) {
	case PRIO_PROCESS:
		if (ap->who == 0) /* current process */
			p = &P;
		else
			p = pfind(ap->who);
		
		if (p)
			nice = p->p_nice;
		break;
	case PRIO_PGRP:
		pgrp = ap->who;
		if (pgrp == 0) /* current process group */
			pgrp = P.p_pgrp;
		
		for EACHPROC(p) {
			if (p->p_pgrp == pgrp && p->p_nice < nice)
				nice = p->p_nice;
		}
		break;
	case PRIO_USER:
		uid = ap->who;
		if (uid == 0) /* current user */
			uid = P.p_uid;
		
		for EACHPROC(p) {
			if (p->p_uid == uid && p->p_nice < nice)
				nice = p->p_nice;
		}
		break;
	default:
		P.p_error = EINVAL;
	}
	
	if (nice <= PRIO_MAX)
		P.p_retval = nice - NZERO;
	else
		P.p_error = ESRCH;
}

STARTUP(void sys_setpriority())
{
	struct a {
		int which;
		id_t who;
		int value;
	} *ap = (struct a *)P.p_arg;
	struct proc *p;
	pid_t pgrp;
	uid_t uid;
	int found = 0;
	
	switch (ap->which) {
	case PRIO_PROCESS:
		if (ap->who == 0) /* current process */
			p = &P;
		else
			p = pfind(ap->who);
		
		if (p) {
			++found;
			donice(p, ap->value);
		}
		break;
	case PRIO_PGRP:
		pgrp = ap->who;
		if (pgrp == 0) /* current process group */
			pgrp = P.p_pgrp;
		
		for EACHPROC(p) {
			if (p->p_pgrp == pgrp) {
				++found;
				donice(p, ap->value);
			}
		}
		break;
	case PRIO_USER:
		uid = ap->who;
		if (uid == 0) /* current user */
			uid = P.p_uid;
		
		for EACHPROC(p) {
			if (p->p_uid == uid) {
				++found;
				donice(p, ap->value);
			}
		}
		break;
	default:
		P.p_error = EINVAL;
	}
	
	if (!found)
		P.p_error = ESRCH;
}

STARTUP(void sys_getrlimit())
{
	struct a {
		int resource;
		struct rlimit *rlp;
	} *ap = (struct a *)P.p_arg;
	
	switch (ap->resource) {
	case RLIMIT_CPU:
	case RLIMIT_DATA:
	case RLIMIT_NOFILE:
	case RLIMIT_CORE:
	case RLIMIT_FSIZE:
	case RLIMIT_STACK:
	case RLIMIT_AS:
		if (copyout(ap->rlp, &P.p_rlimit[ap->resource], sizeof(struct rlimit)))
			P.p_error = EFAULT;
		break;
	default:
		P.p_error = EINVAL;
	}
}

STARTUP(void sys_setrlimit())
{
	struct a {
		int resource;
		const struct rlimit *rlp;
	} *ap = (struct a *)P.p_arg;
	
	/* FIXME: write this! */
	switch (ap->resource) {
	case RLIMIT_CPU:
		break;
	case RLIMIT_DATA:
		break;
	case RLIMIT_NOFILE:
		break;
	case RLIMIT_CORE:
	case RLIMIT_FSIZE:
	case RLIMIT_STACK:
	case RLIMIT_AS:
	default:
		P.p_error = EINVAL;
	}
}

STARTUP(void sys_getrusage())
{
	struct a {
		int who;
		struct rusage *r_usage;
	} *ap = (struct a *)P.p_arg;
	
	P.p_error = ENOSYS;
	return;
	
	if (!ap->r_usage) {
		P.p_error = EFAULT;
		return;
	}
	
	switch (ap->who) {
	case RUSAGE_SELF:
		if (copyout(ap->r_usage, &P.p_rusage, sizeof(struct rusage)))
			P.p_error = EFAULT;
		break;
	case RUSAGE_CHILDREN:
		break;
	default:
		P.p_error = EINVAL;
	}
}
