#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

/* this should be POSIX-compliant now */

#include <sys/time.h>
#include <sys/types.h>

#define PRIO_PROCESS	0
#define PRIO_PGRP	1
#define PRIO_USER	2

/* PRIO_MIN and PRIO_MAX are not in POSIX */
#define PRIO_MIN 0
#define PRIO_MAX (NZERO*2-1)

typedef unsigned long	rlim_t;

#define RLIM_INFINITY	((rlim_t)-1)
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
#define RLIMIT_DATA	1
#define RLIMIT_NOFILE	2
#define RLIMIT_FSIZE	3
#define RLIMIT_CORE	4
#define RLIMIT_STACK	5
#define RLIMIT_AS	6

int	getpriority(int __which, id_t __who);
int	setpriority(int __which, id_t __who, int __value);
int	getrlimit(int __resource, struct rlimit *__rlp);
int	setrlimit(int __resource, const struct rlimit *__rlp);
int	getrusage(int __who, struct rusage *__r_usage);

#endif
