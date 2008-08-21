#ifndef _SYS_TIMES_H_
#define _SYS_TIMES_H_

/* $Id: times.h,v 1.2 2008/01/11 13:33:03 fredfoobar Exp $ */

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
