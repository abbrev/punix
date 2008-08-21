#ifndef _UTIME_H_
#define _UTIME_H_

/* $Id: utime.h,v 1.1 2007/05/03 08:59:58 fredfoobar Exp $ */

#include <sys/types.h>

/* User's structure for utime() system call */
struct utimbuf {
	time_t	actime;		/* Access time. */
	time_t	modtime;	/* Modification time. */
};

extern int	utime(const char *__filename, const struct utimbuf *__buf);

#endif
