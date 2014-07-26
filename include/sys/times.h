#ifndef _SYS_TIMES_H_
#define _SYS_TIMES_H_

/* $Id$ */

#include <sys/types.h>

/* User's structure for times() system call */
struct tms {
	clock_t tms_utime;  /* user CPU time */
	clock_t tms_stime;  /* system CPU time */
	clock_t tms_cutime; /* user CPU time of terminated child processes */
	clock_t tms_cstime; /* system CPU time of terminated child processes */
};

extern clock_t times(struct tms *__buf);

#endif
