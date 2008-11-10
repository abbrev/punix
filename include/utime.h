#ifndef _UTIME_H_
#define _UTIME_H_

/* $Id$ */

#include <sys/types.h>

/* User's structure for utime() system call */
struct utimbuf {
	time_t	actime;		/* Access time. */
	time_t	modtime;	/* Modification time. */
};

extern int	utime(const char *__path, const struct utimbuf *__times);

#endif
