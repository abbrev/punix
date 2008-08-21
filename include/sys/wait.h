#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>

#define WNOHANG		1
#define WUNTRACED	2

#define WEXITSTATUS(s)	(((s) & 0xff00) >> 8)
/* #define WIFCONTINUED(s) */
#define WIFEXITED(s)	(WTERMSIG(s) == 0)
#define WIFSIGNALED(s)	(!WIFSTOPPED(s) && !WIFEXITED(s))
#define WIFSTOPPED(s)	(((s) & 0xff) == 0x7f)
#define WSTOPSIG(s)	WEXITSTATUS(s)
#define WTERMSIG(s)	((s) & 0x7f)

#if 0
/* XXX */
#define WEXITED		1
#define WSTOPPED	2
#define WCONTINUED	3
#define WNOWAIT		4
#endif

typedef enum {P_ALL, P_PID, P_PGID,} idtype_t;

pid_t	wait(int *__status);
int	waitid(idtype_t, id_t, siginfo_t *, int);
pid_t	waitpid(pid_t __pid, int *__status, int __options);

/* BSD syscalls */
pid_t	wait3(int *__status, int __options, struct rusage *__rusage);
pid_t	wait4(pid_t __pid, int *__status, int __options,
		struct rusage *__rusage);

#endif
