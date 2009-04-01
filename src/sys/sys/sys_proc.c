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

#include "setjmp.h"
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

/* XXX: user functions for testing */
extern ssize_t write(int fd, const void *buf, size_t count);
extern void _exit(int status);

extern void userstart();

static void println(char *s)
{
	write(2, s, strlen(s));
	write(2, "\n", 1);
}

int printf(const char *format, ...);

/* simple implementations of some C standard library functions */
time_t time(time_t *tp)
{
	struct timeval tv;
	time_t t;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	if (tp) *tp = t;
	return t;
}

struct tm *gmtime_r(const time_t *tp, struct tm *tmp)
{
	int sec, min, hour, day;
	time_t t = *tp;
	sec = t % 60;  t /= 60;
	min = t % 60;  t /= 60;
	hour = t % 24; t /= 24;
	day = t;
	tmp->tm_sec = sec;
	tmp->tm_min = min;
	tmp->tm_hour = hour;
	/* FIXME: do the rest! */
	return tmp;
}

#define ESC "\x1b"
/* erase from cursor to end of line (inclusive) */
static void cleareol()
{
	printf(ESC "[K");
}

static void updatetop()
{
	struct timeval tv;
	struct timeval difftime;
	long la[3];
	int day, hour, minute, second;
	long msec;
	long umsec, smsec;
	long t;
	int i;
	int ucpu = 42;
	int scpu = 31;
	struct rusage rusage;
	struct timeval ptime;
	struct timeval diffutime;
	struct timeval diffstime;
	
	gettimeofday(&tv, NULL);
	getrusage(RUSAGE_SELF, &rusage);
	
	if (G.lasttime.tv_sec != 0 || G.lasttime.tv_usec != 0) {
		timersub(&tv, &G.lasttime, &difftime);
		timersub(&rusage.ru_utime, &G.lastrusage.ru_utime, &diffutime);
		timersub(&rusage.ru_stime, &G.lastrusage.ru_stime, &diffstime);
		timeradd(&rusage.ru_utime, &rusage.ru_stime, &ptime);
		msec = difftime.tv_sec * 1000L + difftime.tv_usec / 1000;
		umsec = diffutime.tv_sec * 1000L + diffutime.tv_usec / 1000;
		smsec = diffstime.tv_sec * 1000L + diffstime.tv_usec / 1000;
		ucpu = (1000L * umsec + 500) / msec;
		scpu = (1000L * smsec + 500) / msec;
		getloadavg1(la, 3);
		
		/* line 1 */
		t = tv.tv_sec - 25200; /* -7 hours */
		second = t % 60; t /= 60;
		minute = t % 60; t /= 60;
		hour = t % 24;   t /= 24;
		day = t;
		printf(ESC "[H" "%02d:%02d:%02d up ",
		       hour, minute, second);
		t = uptime.tv_sec;
		second = t % 60; t /= 60;
		minute = t % 60; t /= 60;
		hour = t % 24;   t /= 24;
		day = t;
		if (day) {
			printf("%d+", day);
		}
		printf("%02d:%02d, 1 user, load:", hour, minute);
		for (i = 0; i < 3; ++i) {
			if (i > 0) putchar(',');
			printf(" %ld.%02ld", la[i] >> 16,
			       (100 * la[i] >> 16) % 100);
		}
		cleareol();
		/* line 2 */
		printf("\nTasks: 1 total, 1 run, 0 slp, 0 stop, 0 zomb");
		cleareol();
		/* line 3 */
		int totalcpu = ucpu+scpu;
		int idle;
		if (totalcpu > 1000) totalcpu = 1000;
		idle = 1000 - totalcpu;
		printf("\nCpu(s): %3d.%01d%%us, %3d.%01d%%sy, %3d.%01d%%ni, %3d.%01d%%id", ucpu/10, ucpu%10, scpu/10, scpu%10, 0, 0, idle/10, idle%10);
		cleareol();
		/* line 4 */
		printf("\nMem: 0k total, 0k used, 0k free, 0k buffers");
		cleareol();
		/* line 5 */
		putchar('\n');
		cleareol();
		/* line 6 */
		printf("\n" ESC "[7m  PID USER      PR  NI  SHR  RES S  %%CPU   TIME COMMAND" ESC "[m");
		cleareol();
		/* line 7- */
		int pid = getpid();
		/* BOGUS! we can't really use 'current' in userspace */
		printf("\n%5d %-8s %3d %3d %3ldk %3ldk %c %3d.%01d %3d:%02d %.12s",
		       pid, "root", current->p_pri,
		       getpriority(PRIO_PROCESS, pid), (long)0, (long)0, 'R',
		       totalcpu / 10, totalcpu % 10,
		       (int)(ptime.tv_sec / 60), (int)(ptime.tv_sec % 60),
		       "top");
		cleareol();
		printf(ESC "[J" ESC "[5H");
	}
	G.lasttime = tv;
	G.lastrusage = rusage;
}

int usermain(int argc, char *argv[], char *envp[])
{
	int fd;
	int audiofd;
	
	fd = open("/dev/vt", O_RDWR); /* 0 */
	if (fd < 0) _exit(-1);
	dup(fd); /* 1 */
	dup(fd); /* 2 */
	
	println("This is a user program.");
	
	println("\narguments:");
	for (; *argv; ++argv)
		println(*argv);
	
	println("\nenvironment:");
	for (; *envp; ++envp)
		println(*envp);
	
	int i;
	struct timeval starttv;
	struct timeval endtv;
	struct timeval tv;
#if 1
	printf("waiting for the clock to settle...");
	gettimeofday(&starttv, 0);
	do {
		gettimeofday(&tv, 0);
	} while (tv.tv_sec < starttv.tv_sec + 3);
	printf("starting\n");
	gettimeofday(&starttv, 0);
	for (i = 0; i < 8000; ++i) {
		gettimeofday(&endtv, 0);
	}
	tv.tv_sec = endtv.tv_sec - starttv.tv_sec;
	tv.tv_usec = endtv.tv_usec - starttv.tv_usec;
	if (tv.tv_usec < 0) {
		tv.tv_usec += 1000000L;
		tv.tv_sec--;
	}
	long x = tv.tv_usec + 1000000L * tv.tv_sec;
	x /= 8;
	printf("%ld.%06ld = %ld ns per call = %ld calls per second\n", tv.tv_sec, tv.tv_usec, x, 1000000000L / x);
#endif
	
	/* test invalid address */
	printf("testing syscall with an invalid pointer:\n");
	int error = gettimeofday((void *)0x40000 - 1, 0);
	printf("error = %d\n", error);
	
	long divl(long, long);
	unsigned long j;
	volatile unsigned long q;
#if 0
	for (j = 100000-1; j < 5000000; j+=101) {
		q = j / 100000;
		//q = divl(j, 100000);
		if (j < q * 100000 || (q+1) * 100000 <= j)
			printf("%ld %ld %ld\n", j, q, q*100000);
	}
#endif
	for (i = 0; i < 0; ++i) {
		gettimeofday(&starttv, 0);
		for (j = 0; j < 10000000L; j+=100) {
			q = j / 100000;
		}
		gettimeofday(&endtv, 0);
		timersub(&endtv, &starttv, &tv);
		printf("%ld.%06ld\n", tv.tv_sec, tv.tv_usec);
	}
	
	printf("Testing the tty.\nType ctrl-d to end input and go to the next test.\n");
	/* cat */
	char buf[60];
	ssize_t n;
	for (;;) {
		n = read(0, buf, 60);
		//printf("%ld\n", n);
		if (n <= 0) break;
		fwrite(buf, n, (size_t)1, NULL);
		fflush(NULL);
	}
	
	audiofd = open("/dev/audio", O_RDWR);
	printf("audiofd = %d\n", audiofd);
	
	long audio[1024];
	for (n = 0; n < 1024; ++n) audio[n] = 0x00cccc00;
	for (n = 0; n < 1024; ++n) write(audiofd, audio, 4);
	write(audiofd, audio, sizeof(audio));
	
	long lasttime = 0;
	printf(ESC "[H" ESC "[J");
	for (;;) {
		gettimeofday(&tv, NULL);
		if (tv.tv_sec > lasttime && (tv.tv_sec % 3) == 0)
			updatetop();
		lasttime = tv.tv_sec;
	}
	
	return 0;
}
/* XXX */

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
	wakeup(pp);
	while (!(P.p_flag&P_VFDONE))
		slp(pp, PZERO-1);
#if 0
	P.p_dsize = pp->p_dsize = 0; /* are these necessary? */
	P.p_ssize = pp->p_ssize = 0; /* " */
#endif
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
	 * XXX: this version does not load the binary from a file, allocate
	 * memory, or any of that fun stuff (as these capabilities have not
	 * been implemented in Punix yet).
	 */
	
	
	void *text = userstart; /* XXX */
	char *data = NULL;
	char *stack = NULL;
	char *ustack = NULL;
	size_t textsize;
	size_t datasize;
	size_t stacksize;
	size_t ustacksize;
	
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
	
#define STACKSIZE 1024
#define USTACKSIZE 1024
#define UDATASIZE 1024
	/* allocate the system stack */
	stacksize = STACKSIZE;
	stack = memalloc(&stacksize, 0);
	/* if (!stack) goto ... */
	/* allocate the user stack */
	ustacksize = USTACKSIZE;
	ustack = memalloc(&ustacksize, 0);
	/* if (!ustack) goto ... */
	ustack += ustacksize;
	
	/* allocate the data section */
	datasize = UDATASIZE;
	data = memalloc(&datasize, 0);
	/* if (!data) goto ... */
	
	size = (size + 1) & ~1; /* size must be even */
	s = ustack - size; /* start of strings */
	v = (char **)s - (argc + envc + 2); /* start of vectors */
	
	ustack = (char *)v;
	*--(int *)ustack = argc;
	
	/* copy arguments */
	copyenv(&v, &s, argp);
	copyenv(&v, &s, envp);
	
	if (P.p_flag & P_VFORK)
		endvfork();
	else {
		/* free our resources */
#if 1
		memfree(NULL, P.p_pid);
		if (P.p_stack)
			memfree(P.p_stack, 0);
		if (P.p_ustack)
			memfree(P.p_ustack, 0);
		if (P.p_text)
			memfree(P.p_text, 0);
		if (P.p_data)
			memfree(P.p_data, 0);
#endif
	}
	P.p_stack = stack;
	P.p_ustack = ustack;
	P.p_text = text;
	P.p_data = data;
	
	/* finally, set up the context to return to the new process image */
	if (setjmp(P.p_ssav))
		return;
	
	/* go to the new user context */
	P.p_ssav->usp = ustack;
	P.p_sfp->pc = text;
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

void doexit(int status)
{
	int i;
	struct proc *q;
	
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
	iput(P.p_cdir);
	if (P.p_rdir) {
		ilock(P.p_rdir);
		iput(P.p_rdir);
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
	
	if (P.p_pid == 1)
		panic("init died");
	
	P.p_waitstat = status;
	/* ruadd(&P.p_rusage, &P.p_crusage); */
	for EACHPROC(q) {
		if (q->p_pptr == &P) {
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
	wakeup(P.p_pptr);
	swtch();
}

/* XXX: fix this to conform to the new vfork() method */
void sys_exit()
{
	struct a {
		int status;
	} *ap = (struct a *)P.p_arg;
	
	doexit(W_EXITCODE(ap->status, 0));
}

void sys_fork()
{
	P.p_error = ENOSYS;
}

void sys_vfork()
{
#if 0
	struct proc *cp = NULL;
	pid_t pid;
	void *sp = NULL;
	void setup_env(jmp_buf env, struct syscallframe *sfp, long *sp);
	
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
	cp->p_status = P_RUNNING; /* FIXME: use setrun() instead */
	cp->p_flag |= P_VFORK;
	
	/* use the new kernel stack but the same user stack */
	setup_env(cp->p_ssav, P.p_sfp, sp);
	
	/* wait for child to end its vfork */
	while (cp->p_flag & P_VFORK)
		slp(cp, PSWP+1);
	
	/* reclaim our stuff */
	cp->p_flag |= P_VFDONE;
	wakeup(&P);
	return;
stackp:
	pfree(cp);
nomem:
#endif
	P.p_error = ENOMEM;
}

static void dowait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	struct proc *p;
	int nfound = 0;
	int s;
	struct rusage r;
	int error;
	
loop:
	for EACHPROC(p)
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
	
	donice(&P, P.p_nice + ap->inc);
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
			uid = P.p_euid;
		
		for EACHPROC(p) {
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
			uid = P.p_euid;
		
		for EACHPROC(p) {
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
	
/*
	P.p_error = ENOSYS;
	return;
*/
	
	switch (ap->who) {
	case RUSAGE_SELF:
		if (copyout(ap->r_usage, &P.p_rusage, sizeof(struct rusage)))
			P.p_error = EFAULT;
		break;
	case RUSAGE_CHILDREN:
		P.p_error = EINVAL; /* FIXME */
		break;
	default:
		P.p_error = EINVAL;
	}
}
