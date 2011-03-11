/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
 * 
 * $Id$
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

#define _BSD_SOURCE

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sound.h>
#include <sys/utsname.h>
#include <setjmp.h>
#include <assert.h>

#include "proc.h"
#include "punix.h"
#include "process.h"
#include "queue.h"
#include "inode.h"
#include "wait.h"
#include "globals.h"

/* process manipulation system calls */

void sys_sigreturn()
{
	/* FIXME: write this! */
}

static void copyenv(char ***vp, char **sp, char **sv)
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

static void endvfork()
{
	struct proc *pp;
	
	pp = P.p_pptr;
	P.p_flag &= ~P_VFORK;
	wakeup(current);
#if 0
	while (!(P.p_flag&P_VFDONE))
		slp(pp, 0);
#endif
#if 0
	P.p_dsize = pp->p_dsize = 0; /* are these necessary? */
	P.p_ssize = pp->p_ssize = 0; /* " */
#endif
}

static void *stackalloc(size_t *size)
{
	void *p = memalloc(size, 0);
	if (p)
		p += *size; /* stack starts at the top */
	return p;
}

/*
 * release the resources of this process
 * if we are vfork'd, just end the vfork
 * otherwise free our memory
 */
static void release_resources()
{
	if (P.p_flag & P_VFORK) {
		endvfork();
	} else {
		/* free our resources */
#if 1
		memfree(NULL, P.p_pid); /* free all user allocations */
		if (P.p_ustack) {
			memfree(P.p_ustack, 0);
			P.p_ustack = NULL;
		}
		/*
		if (P.p_text)
			memfree(P.p_text, 0);
		*/
		if (P.p_data) {
			memfree(P.p_data, 0);
			P.p_data = NULL;
		}
#endif
	}
}

/* push a long/word/byte onto a stack */
#define PUSHL(stack, value) (*--((long *)(stack)) = (value))
#define PUSHW(stack, value) (*--((short *)(stack)) = (value))
#define PUSHB(stack, value) (*--((char *)(stack)) = (value))

/* XXX: these values need to be loaded from the binary file */
#define STACKSIZE 1024
#define USTACKSIZE 2048
#define UDATASIZE 1

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
void sys_execve()
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
	 * XXX: this version does not load the binary from a file yet
	 */
	
	
	void *text = NULL;
	char *data = NULL;
	char *ustack = NULL;
	char *ustackstart = NULL;
	size_t textsize;
	size_t datasize;
	size_t ustacksize;
	
	long size = 0;
	char **ap;
	char *s, **v;
	int argc, envc;
	
	/* XXX */
	void sh_start(), init_start(), bittybox_start();
	if (!strcmp(pathname, "init") || !strcmp(pathname, "/etc/init"))
		text = init_start;
	else if (!strcmp(pathname, "cat") ||
	         !strcmp(pathname, "top") ||
	         !strcmp(pathname, "false") ||
	         !strcmp(pathname, "true") ||
	         !strcmp(pathname, "clear") ||
	         !strcmp(pathname, "uname") ||
	         !strcmp(pathname, "id") ||
	         !strcmp(pathname, "batt") ||
	         !strcmp(pathname, "date") ||
	         !strcmp(pathname, "adjtime"))
		text = bittybox_start;
	else if (!strcmp(pathname, "sh"))
		text = sh_start;
	else {
		P.p_error = ENOENT;
		goto error_noent;
	}
	
	/* set up the user's stack (argc, argv, and env) */
	
	/* calculate the size of the arguments and environment + the size of
	 * their vectors */
	
	for (ap = argp; *ap; ++ap)
		size += strlen(*ap) + 1;
	argc = ap - argp; /* number of arg vectors */
	size += (argc+1)*sizeof(char *);
	
	for (ap = envp; *ap; ++ap)
		size += strlen(*ap) + 1;
	envc = ap - envp; /* number of env vectors */
	size += (envc+1)*sizeof(char *);

	size += sizeof(int); /* for argc */
	
	/* allocate the user stack */
	ustacksize = USTACKSIZE + size;
	ustack = stackalloc(&ustacksize);
	if (!ustack) {
		goto error_ustack;
	}
	//kprintf("execve(): ustack=%08lx\n", ustack);
	ustackstart = ustack - ustacksize;
	
	/* allocate the data section */
	datasize = UDATASIZE;
	data = memalloc(&datasize, 0);
	if (!data) {
		goto error_data;
	}
	//kprintf("execve():   data=%08lx\n", data);
	//printheaplist();
	
	/*
	 * at this point we have all memory areas allocated. set up the new
	 * process image
	 */

	/* copy the first argument (program name) to the p_name field */
	strncpy(current->p_name, argp[0], P_NAMELEN - 1);
	current->p_name[P_NAMELEN-1] = '\0';


	size = (size + 1) & ~1; /* size must be even */
	s = ustack - size; /* start of strings */
	v = (char **)s - (argc + envc + 2); /* start of vectors */
	
	ustack = (char *)v;
	PUSHW(ustack, argc);
	
	/* copy arguments and environment */
	copyenv(&v, &s, argp);
	copyenv(&v, &s, envp);
	
	/* free our old user stack and data segment */
	release_resources();
	P.p_ustack = ustackstart;
	P.p_text = text;
	P.p_data = data;
	
	//P.p_datasize = datasize;
	
	/* finally, set up the context to return to the new process image */
	
	/* go to the new user context */
	P.p_syscall_ctx->pc = text;
	P.p_syscall_ctx->usp = ustack;
	
	return;
	
	/*
	 * following comments are out of date:
	 *
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
error_data:
	memfree(ustackstart, 0);
error_ustack: ;
error_noent: ;
}

void doexit(int status)
{
	int i;
	struct proc *q;
	size_t zombsize = sizeof(struct zombproc);
	
	P.p_flag &= ~P_TRACED;
	P.p_sigignore = ~0;
	P.p_sig = 0;
	
#if 0
	for (i = 0; i < NOFILE; ++i) {
		struct file *f;
		f = P.p_ofile[i];
		P.p_ofile[i] = NULL;
		P.p_oflag[i] = 0;
		closef(f); /* crash in closef() */
	}
	ilock(P.p_cdir);
	i_unref(P.p_cdir);
	if (P.p_rdir) {
		ilock(P.p_rdir);
		i_unref(P.p_rdir);
	}
	P.p_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	if (P.p_flag & P_VFORK)
		endvfork();
	else {
		/* xfree(); */
		/* mfree(...); */
	}
	/* mfree(...); */
#endif
	
	if (P.p_pid == 1) {
#if 1
		if (WIFEXITED(status))
			kprintf("init exited normally with status %d\n", WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			kprintf("init terminated with signal %d\n", WTERMSIG(status));
		else if (WIFSTOPPED(status))
			kprintf("init stopped with signal %d\n", WSTOPSIG(status));
		else if (WIFCONTINUED(status))
			kprintf("init continued\n");
		else
			kprintf("init unknown status 0x%04x!\n", status);
#endif

		panic("init died");
	}
	
	P.p_waitstat = status;
	/* add our children's rusage to our own */
	ruadd(&P.p_rusage, &P.p_crusage);
	list_for_each_entry(q, &G.proc_list, p_list) {
		if (q->p_pptr == current) {
			q->p_pptr = G.initproc;
			wakeup(G.proclist);
			if (q->p_flag & P_TRACED) {
				q->p_flag &= ~P_TRACED;
				psignal(q, SIGKILL);
			} else if (q->p_status == P_STOPPED) {
				psignal(q, SIGHUP);
				psignal(q, SIGCONT);
			}
		}
	}
	psignal(P.p_pptr, SIGCHLD);
	release_resources();
	memfree(P.p_stack, 0);
	P.p_stack = NULL;
	struct zombproc *zp;
	zp = memrealloc(current, &zombsize, MEMREALLOC_TOP, 0);
	/* if this assert fails, there is a problem with the allocator */
	assert(zp == current);

	sched_exit(current);
	swtch();
}

void sys_pause()
{
	slp(sys_pause, 1);
}

/* XXX: fix this to conform to the new vfork() method */
void sys_exit()
{
	struct a {
		int status;
	} *ap = (struct a *)P.p_arg;
	
	doexit(W_EXITCODE(ap->status));
}

void sys_fork()
{
	P.p_error = ENOSYS;
}

/*
 * memory allocations and frees:
 * vfork:
 * 	allocate kernel stack
 * execve:
 * 	allocate new user stack and data segment
 * 	free old user stack and data segment if not vfork
 * exit:
 * 	free user stack and data segment if not vfork
 * 	free kernel stack
 */

void sys_vfork()
{
#if 1
	struct proc *cp = NULL;
	pid_t pid;
	void *stack = NULL;
	size_t stacksize = STACKSIZE;
	void setup_env(struct context *, struct syscallframe *sfp, long *sp);
	
	/* XXX: remove this block once vfork is completely written */
	if (0) {
		P.p_error = ENOMEM;
		return;
	}
	
	/* spl7(); */
	
	cp = palloc();
	if (!cp)
		goto nomem;
	
	*cp = *current; /* copy our own process structure to the child */
	
	pid = pidalloc();
	P.p_retval = pid;
	cp->p_pid = pid;
	cp->p_pptr = current;
	cp->p_rusage.ru_utime.tv_sec =
	 cp->p_rusage.ru_utime.tv_usec =
	 cp->p_rusage.ru_stime.tv_sec =
	 cp->p_rusage.ru_stime.tv_usec = 0;
	cp->p_crusage.ru_utime.tv_sec =
	 cp->p_crusage.ru_utime.tv_usec =
	 cp->p_crusage.ru_stime.tv_sec =
	 cp->p_crusage.ru_stime.tv_usec = 0;
	
	/* allocate a kernel stack for the child */
	stack = stackalloc(&stacksize);
	if (!stack)
		goto stackp;
	//kprintf("vfork(): stack=%08lx\n", stack);
	
	/* At this point there's no turning back! */
	
	list_add_tail(&cp->p_list, &G.proc_list);
	
	cp->p_stack = stack - stacksize;
	cp->p_flag |= P_VFORK;
	cp->p_status = P_VFORKING;

	void return_from_vfork();
	cp->p_ctx = *P.p_syscall_ctx;
	//kprintf("vfork(): ctx.pc =%08lx\n", cp->p_ctx.pc);

	/* push return address and SR onto the stack */
	PUSHL(stack, (long)cp->p_ctx.pc);
	PUSHW(stack, 0x0000); /* user mode SR */
	PUSHL(stack, 0); /* dummy return address */
	
	cp->p_ctx.pc = return_from_vfork;
	cp->p_ctx.sp = stack;
	cp->p_ctx.sr = 0x2000; /* supervisor mode SR */
	sched_fork(cp);

	/* wait for child to end its vfork */
	while (cp->p_flag & P_VFORK)
		slp(cp, 0);
	
	/* reclaim our stuff */
#if 0
	cp->p_flag |= P_VFDONE;
	wakeup(current); /* ??? */
#endif
	return;
stackp:
	pfree(cp);
nomem:
#endif
}

#if 0
/* current rusage structure */
struct rusage {
	struct timeval ru_utime;        /* user time used */
	struct timeval ru_stime;        /* system time used */
	long           ru_nvcsw;        /* voluntary context switches */
	long           ru_nsignals;
};
#endif

/* convert kernel rusage to user rusage */
static void ru_to_user_ru(struct rusage *dest, struct rusage *src)
{
	*dest = *src;
	dest->ru_utime.tv_usec = src->ru_utime.tv_usec * 1000000L / HZ;
	dest->ru_stime.tv_usec = src->ru_stime.tv_usec * 1000000L / HZ;
}

/* NB: rusage times are stored as seconds:ticks */
void ruadd(struct rusage *dest, struct rusage *src)
{
	dest->ru_utime.tv_sec += src->ru_utime.tv_sec;
	dest->ru_utime.tv_usec += src->ru_utime.tv_usec;
	if (dest->ru_utime.tv_usec >= HZ) {
		dest->ru_utime.tv_usec -= HZ;
		++dest->ru_utime.tv_sec;
	}
	dest->ru_stime.tv_sec += src->ru_stime.tv_sec;
	dest->ru_stime.tv_usec += src->ru_stime.tv_usec;
	if (dest->ru_stime.tv_usec >= HZ) {
		dest->ru_stime.tv_usec -= HZ;
		++dest->ru_stime.tv_sec;
	}
	dest->ru_nvcsw += src->ru_nvcsw;
	dest->ru_nsignals += src->ru_nsignals;
}

static void dowait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	struct proc *p;
	int nfound = 0;
	int error;
	
loop:
	list_for_each_entry(p, &G.proc_list, p_list)
	if (p->p_pptr == current) {
		if (pid > 0) {
			if (p->p_pid != pid) continue;
		} else if (pid == 0) {
			if (p->p_pgrp != P.p_pgrp) continue;
		} else if (pid < (pid_t)-1) {
			if (p->p_pgrp != -pid) continue;
		}
		++nfound;
		if (P.p_flag & P_NOCLDWAIT)
			continue;
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
	error = tsleep(current, PWAIT|PCATCH, 0);
	if (!error) goto loop;
	P.p_error = error;
	return;
	
found:
	P.p_retval = p->p_pid;
	if (status) {
		if (copyout(status, &p->p_waitstat, sizeof(*status)))
			P.p_error = EFAULT;
	}
	
	if (rusage) {
		/* fill in rusage */
		struct rusage r;
		ru_to_user_ru(&r, &p->p_rusage);
		if (copyout(rusage, &r, sizeof(*rusage)))
			P.p_error = EFAULT;
	}
	
	if (p->p_status == P_ZOMBIE) {
		ruadd(&current->p_crusage, &p->p_rusage);
		list_del(&p->p_list);
		pfree(p);
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

void sys_wait()
{
	struct wait3a *ap = (struct wait3a *)P.p_arg;
	dowait4(-1, ap->status, 0, NULL);
}

void sys_waitpid()
{
	struct wait4a *ap = (struct wait4a *)P.p_arg;
	dowait4(ap->pid, ap->status, ap->options, NULL);
}

/*
 * The following two system calls are XSI extensions and are not necessary for
 * POSIX conformance.
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

void sys_getpid()
{
	P.p_retval = P.p_pid;
}

void sys_getppid()
{
	P.p_retval = P.p_pptr->p_pid;
}

static void donice(struct proc *p, int n)
{
	if (P.p_euid != 0 && P.p_ruid != 0 &&
	    P.p_euid != p->p_euid && P.p_ruid != p->p_euid) {
		P.p_error = EPERM;
		return;
	}
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	if (n < p->p_nice && P.p_euid != 0) {
		P.p_error = EACCES;
		return;
	}
	p->p_nice = n;
}

void sys_nice()
{
	struct a {
		int inc;
	} *ap = (struct a *)P.p_arg;
	
	donice(current, P.p_nice + ap->inc);
	P.p_retval = P.p_nice - NZERO;
	
	/*
	 * setpriority() returns EACCES if process
	 * attempts to lower the nice value if the
	 * user does not have the appropriate
	 * privileges, but nice() returns EPERM
	 * instead. Curious.
	 */
	if (P.p_error == EACCES)
		P.p_error = EPERM;
}

void sys_getpriority()
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
			p = current;
		else
			p = pfind(ap->who);
		
		if (p)
			nice = p->p_nice;
		break;
	case PRIO_PGRP:
		pgrp = ap->who;
		if (pgrp == 0) /* current process group */
			pgrp = P.p_pgrp;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_pgrp == pgrp && p->p_nice < nice)
				nice = p->p_nice;
		}
		break;
	case PRIO_USER:
		uid = ap->who;
		if (uid == 0) /* current user */
			uid = P.p_euid;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_euid == uid && p->p_nice < nice)
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

void sys_setpriority()
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
			p = current;
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
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_pgrp == pgrp) {
				++found;
				donice(p, ap->value);
			}
		}
		break;
	case PRIO_USER:
		uid = ap->who;
		if (uid == 0) /* current user */
			uid = P.p_euid;
		
		list_for_each_entry(p, &G.proc_list, p_list) {
			if (p->p_euid == uid) {
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

void sys_getrlimit()
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

void sys_setrlimit()
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

void sys_getrusage()
{
	struct a {
		int who;
		struct rusage *r_usage;
	} *ap = (struct a *)P.p_arg;
	
	int x;
	struct rusage ru, *rup;
	
	switch (ap->who) {
	case RUSAGE_SELF:
		rup = &current->p_rusage;
		break;
	case RUSAGE_CHILDREN:
		rup = &current->p_crusage;
		break;
	default:
		P.p_error = EINVAL;
		goto error;
	}
	
	x = spl1();
	memmove(&ru, rup, sizeof(ru));
	splx(x);
	ru.ru_utime.tv_usec = ru.ru_utime.tv_usec * 1000000L / HZ;
	ru.ru_stime.tv_usec = ru.ru_stime.tv_usec * 1000000L / HZ;
	if (copyout(ap->r_usage, &ru, sizeof(ru)))
		P.p_error = EFAULT;
error:	;
}

void sys_uname()
{
	struct a {
		struct utsname *name;
	} *ap = (struct a *)P.p_arg;
	
	struct utsname me;
#define COPY(name) strncpy(me.name, uname_##name, sizeof(me.name))
	COPY(sysname);
	COPY(nodename);
	COPY(release);
	COPY(version);
	COPY(machine);
#undef COPY
	P.p_error = copyout(ap->name, &me, sizeof(me));
}

void sys_chdir()
{
	/*  int chdir(const char *path); */
	struct a {
		const char *path;
	} *ap = (struct a *)P.p_arg;
	struct inode *ip = NULL;
	
#ifdef NOTYET
	ip = namei(ap->path);
#endif
	if (!ip)
		return;
	if (!(ip->i_mode & IFDIR)) {
		P.p_error = ENOTDIR;
		return;
	}
	if (ip == P.p_cdir) return;
	i_unref(P.p_cdir);
	++ip->i_count; /* unless namei() does this ?? */
	P.p_cdir = ip;
}
