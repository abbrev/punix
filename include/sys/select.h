#ifndef _SYS_SELECT_H_
#define _SYS_SELECT_H_

/* $Id: select.h,v 1.2 2008/01/11 13:30:35 fredfoobar Exp $ */

#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
	unsigned long fds_bits[1];	/* XXX */
} fd_set;

/* define the following macros: */
#if 0
void FD_CLR(int fd, fd_set *fdset);
int FD_ISSET(int fd, fd_set *fdset);
void FD_SET(int fd, fd_set *fdset);
void FD_ZERO(fd_set *fdset);
#endif

#define FD_SETSIZE 32 /* XXX */

int pselect(int __nfds, fd_set *__readfds, fd_set *__writefds,
            fd_set *__errorfds, const struct timespec *__timeout,
            const sigset_t *__sigmask);
int select(int __nfds, fd_set *__readfds, fd_set *__writefds,
           fd_set *__errorfds, struct timeval *__timeout);

#endif
