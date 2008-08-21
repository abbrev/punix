#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

/* this should be POSIX-compliant now */

/* nowhere in POSIX is PRIO_MIN nor PRIO_MAX. Strange? */

#include <sys/time.h>
#include <sys/types.h>

#define PRIO_PROCESS	0
#define PRIO_PGRP	1
#define PRIO_USER	2

typedef unsigned long	rlim_t;

#define RLIM_INFINITY	((unsigned long)-1)
#define RLIM_SAVED_MAX	RLIM_INFINITY
#define RLIM_SAVED_CUR	RLIM_INFINITY

#define RUSAGE_SELF	0
#define RUSAGE_CHILDREN	1

struct rlimit {
	rlim_t	rlim_cur;	/* current (soft) limit */
	rlim_t	rlim_max;	/* hard limit */
};

struct rusage {
	struct timeval	ru_utime;	/* user time used */
	struct timeval	ru_stime;	/* system time used */
};

/* possible values for resource argument of getrlimit() and setrlimit() */
#define RLIMIT_CPU	0
#define RLIMIT_FSIZE	1
#define RLIMIT_DATA	2
#define RLIMIT_STACK	3
#define RLIMIT_CORE	4
#define RLIMIT_NOFILE	5
#define RLIMIT_AS	6

int	getpriority(int __which, id_t __who);
int	getrlimit(int __resource, struct rlimit *__rlp);
int	getrusage(int __who, struct rusage *__r_usage);
int	setpriority(int __which, id_t __who, int __value);
int	setrlimit(int __resource, const struct rlimit *__rlp);

#endif
