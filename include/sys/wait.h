#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>

#define WIFEXITED(s)   (WTERMSIG(s) == 0)
#define WEXITSTATUS(s) (((s) & 0xff00) >> 8)

#define WIFSIGNALED(s) (((signed char) (((s) & 0x7f) + 1) & ~1) > 0)
#define WTERMSIG(s)    ((s) & 0x7f)

#define WIFSTOPPED(s)  (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s)    WTERMSIG(s)

#define WIFCONTINUED(s)     ((s) == 0xff)

/* flags for "options" argument */
#define WNOHANG    001
#define WUNTRACED  002

#define WSTOPPED   002
#define WCONTINUED 004
#define WEXITED    010
#define WNOWAIT    020 /* don't reap child */

typedef enum {P_ALL, P_PID, P_PGID,} idtype_t;

pid_t	wait(int *__status);
pid_t	waitpid(pid_t __pid, int *__status, int __options);

/* XSI */
int	waitid(idtype_t, id_t, siginfo_t *, int);

/* BSD syscalls */
pid_t	wait3(int *__status, int __options, struct rusage *__rusage);
pid_t	wait4(pid_t __pid, int *__status, int __options,
		struct rusage *__rusage);

#endif
