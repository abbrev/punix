#ifndef _SIGNAL_H_
#define _SIGNAL_H_

/* $Id: signal.h,v 1.3 2008/01/11 13:35:45 fredfoobar Exp $ */

/* mostly POSIX compliant */

#include <sys/types.h>
#include <time.h>	/* [CX] optional */

/* for mcontext_t */
/*
#include <ucontext.h>
*/

/* Type of a signal handler.  */
typedef void (*sig_t)(int);
typedef void (*sighandler_t)(int);

#define SIG_DFL  ((sighandler_t)0)	/* default signal handling */
#define SIG_IGN  ((sighandler_t)1)	/* ignore signal */
#define SIG_ERR  ((sighandler_t)-1)	/* error return from signal */
#define SIG_HOLD ((sighandler_t)2)	/* request that signal be held */

typedef volatile int	sig_atomic_t;
typedef unsigned long	sigset_t;

#define NSIG	32

/* signal values */
#define SIGHUP	1 		/* Hangup detected on controlling terminal
				   or death of a controlling process */
#define SIGINT	2 		/* Interrupt from keyboard */
#define SIGQUIT	3		/* Quit from keyboard */
#define SIGILL	4 		/* Illegal instruction */
#define     ILL_RESAD_FAULT	0x0	/* reserved addressing fault */
#define     ILL_PRIVIN_FAULT	0x1	/* privileged instruction fault */
#define     ILL_RESOP_FAULT	0x2	/* reserved operand fault */
#define SIGTRAP	5		/* Trace/breakpoint trap */	/* [XSI] */
#define SIGABRT	6 		/* Abort signal from abort */
#define SIGIOT	SIGABRT		/* IOT trap. A synonym for SIGABRT */
/*#define SIGBUS	7*/ 		/* Bus error */
#define SIGFPE	8 		/* Floating point exception */
#define     FPE_INTOVF_TRAP	0x1	/* integer overflow */
#define     FPE_INTDIV_TRAP	0x2	/* integer divide by zero */
#define     FPE_FLTOVF_TRAP	0x3	/* floating overflow */
#define     FPE_FLTDIV_TRAP	0x4	/* floating/decimal divide by zero */
#define     FPE_FLTUND_TRAP	0x5	/* floating underflow */
#define     FPE_DECOVF_TRAP	0x6	/* decimal overflow */
#define     FPE_SUBRNG_TRAP	0x7	/* subscript out of range */
#define     FPE_FLTOVF_FAULT	0x8	/* floating overflow fault */
#define     FPE_FLTDIV_FAULT	0x9	/* divide by zero floating fault */
#define     FPE_FLTUND_FAULT	0xA	/* floating underflow fault */
#define SIGKILL	9		/* Kill signal */
#define SIGBUS	10		/* Bus error */
#define SIGSEGV	11		/* Invalid memory reference */
#define SIGSYS	12		/* Bad argument to system call [XSI] */
#define SIGPIPE	13		/* Broken pipe: write to pipe with no readers */
#define SIGALRM	14		/* Timer signal from alarm */
#define SIGTERM	15		/* Termination */
#define SIGURG	16		/* Urgent condition on IO channel */
#define SIGSTOP	17		/* Stop process */
#define SIGTSTP	18		/* Keyboard stop */
#define SIGCONT	19		/* Continue process */
#define SIGCHLD	20		/* Child has terminated or stopped */
#define SIGCLD	SIGCHLD
#define SIGTTIN	21		/* */
#define SIGTTOU	22		/* */
#define SIGIO	23		/* Input/output is possible */

#define SIGXCPU	24	/* exceeded CPU time limit */	/* [XSI] */
#define SIGXFSZ	25	/* exceeded file size limit */	/* [XSI] */

#define SIGVTALRM	26	/* Virtual time alarm */	/* [XSI] */

#define SIGPROF	27	/* profiling time alarm */	/* [XSI] */
#define SIGWINCH 28	/* window size changes */

#define SIGUSR1	30 		/* User-defined 1 */
#define SIGUSR2	31		/* User-defined 2 */

/* typedef int signal_t; */

#define sigmask(n)	((unsigned long)1 << ((n)-1)) /* convert signal number to mask */

struct sigvec {
	sighandler_t	sv_handler;
	long		sv_mask;
	int		sv_flags;
};

/* bits for `sigvec::sv_flags' */
#define SV_ONSTACK	(1<<0)	/* take signal on signal stack */
#define SV_INTERRUPT	(1<<1)	/* do not restart system calls */
#define SV_RESETHAND	(1<<2)	/* reset handler to SIG_DFL on receipt */

/* values for `how' argument in `sigprocmask' */
#define SIG_BLOCK	0	/* add signals in set to current set */
#define SIG_UNBLOCK	1	/* remove signals in set from current set */
#define SIG_SETMASK	2	/* copy set to current set */

typedef struct {
	void	*ss_sp;		/* base of stack */
	size_t	ss_size;	/* number of bytes in stack */
	int	ss_flags;
} stack_t;

struct sigstack {
	int	ss_onstack;
	void	*ss_sp;
};

/* values for `sigstack::ss_onstack' */
#define SS_ONSTACK	(1<<0)
#define SS_DISABLE	0

/*
#define MINSIGSTKSZ	
#define SIGSTKSZ	
*/

/* FIXME: clean up this struct */
typedef struct {
	int	si_signo;	/* [CX] */
	int	si_code;	/* [CX] */
	int	si_errno;	/* [XSI] */
	
	union {
		/* kill() */
		struct {
			pid_t	si_pid;	/* [XSI] */
			uid_t	si_uid;	/* [XSI] */
		} _kill;
		
		/* SIGCHLD */
		struct {
			pid_t	si_pid;	/* [XSI] */
			uid_t	si_uid;	/* [XSI] */
			int	si_status;	/* [XSI] */
		} _sigchld;
		
		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void	*si_addr;	/* [XSI] */
		} _sigfault;
		
		/* SIGPOLL */
		struct {
			long	si_band;	/* [XSI] */
		} _sigpoll;
	} _sifields;
} siginfo_t;

#define si_pid		_sifields._kill.si_pid
#define si_uid		_sifields._kill.si_uid
#define si_addr		_sifields._sigfault.si_addr
#define si_status	_sifields._sigchld.si_status
#define si_band		_sifields._sigpoll.si_band

/* values for `siginfo::si_code' */
/*
 * FIXME: insert ILL_*, FPE_*, SEGV_*, BUS_*, TRAP_*, CLD_*, and POLL_*
 * defines here (see IEEE 1003.1).
 */
#define SI_ASYNCNL	(-6)	/* asynchronous name lookup completion */
#define SI_SIGIO	(-5)	/* queued SIGIO */
#define SI_ASYNCIO	(-4)	/* AIO completion [CX] */
#define SI_MESGQ	(-3)	/* real time mesq state change [CX] */
#define SI_TIMER	(-2)	/* timer expiration [CX] */
#define SI_QUEUE	(-1)	/* sigqueue [CX] */
#define SI_USER		(0)	/* kill, sigsend, or raise [CX] */
#define SI_KERNEL	0x80	/* kernel */

struct sigaction {
	union {
		/* used when SA_SIGINFO is not set in sa_flags */
		sighandler_t	sa_handler;
		
		/* used when SA_SIGINFO is set in sa_flags */
		void		(*sa_sigaction)(int, siginfo_t *, void *);
	};
	sigset_t	sa_mask;
	int		sa_flags;
#if 0
	void		(*sa_restorer)(void);
#endif
};

/* bits for `sigaction::sa_flags' */
/* these might be subject to change */
#define SA_NOCLDSTOP	(1<<0)	/* don't send SIGCHLD when a child stops */
#define SA_NOCLDWAIT	(1<<1)	/* don't create zombie when a child dies */
#define SA_SIGINFO	(1<<2)	/* use three-argument handler */
#define SA_NODEFER	(1<<3)	/* don't block a signal upon entry of its
				   handler */
#define SA_NOMASK	SA_NODEFER
#define SA_RESETHAND	(1<<4)	/* reset handler to SIG_DFL on receipt */
#define SA_ONESHOT	SA_RESETHAND
#define SA_ONSTACK	(1<<5)
#define SA_RESTART	(1<<6)

int	kill(pid_t __pid, int __sig);
int	raise(int __sig);
int	sigaction(int __signum, const struct sigaction *__act,
		struct sigaction *__oldact);
int	sigaddset(sigset_t *__set, int __signum);
int	sigaltstack(const stack_t *__ss, stack_t *__oss);
int	sigdelset(sigset_t *__set, int __signum);
int	sigemptyset(sigset_t *__set);
int	sigfillset(sigset_t *__set);
int	sigismember(const sigset_t *__set, int __signum);
sighandler_t	signal(int __signum, sighandler_t __handler);
int	sigpending(sigset_t *__set);
int	sigprocmask(int __how, const sigset_t *__set, sigset_t *__oldset);
int	sigsuspend(const sigset_t *__mask);
int	sigwait(const sigset_t *__set, int *__signum);

extern char	*sys_siglist[];

#endif /* _SIGNAL_H_ */
