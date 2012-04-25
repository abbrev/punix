#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

/* $Id$ */

#include <sys/types.h>

struct timeval {
	time_t		tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* microseconds */
};

struct itimerval {
	struct timeval it_interval; /* timer interval */
	struct timeval it_value;    /* current value */
};

/* values for 'which' argument of getitimer() and setitimer() */
#define ITIMER_REAL    0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF    2

int getitimer(int __which, struct itimerval *__value);
int gettimeofday(struct timeval *__tp, void *__tzp);
int setitimer(int __which, const struct itimerval *__value,
              struct itimerval *__ovalue);
int utimes(char *__path, struct timeval *__tvp);

#ifdef _BSD_SOURCE

void timeradd(struct timeval *__a, struct timeval *__b, struct timeval *__res);
void timersub(struct timeval *__a, struct timeval *__b, struct timeval *__res);

#define timerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0
#define timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)
#define timercmp(tvp, uvp, cmp) \
        ((tvp)->tv_sec == (uvp)->tv_sec ? \
         (tvp)->tv_usec cmp (uvp)->tv_usec : \
         (tvp)->tv_sec cmp (uvp)->tv_sec)

#endif /* _BSD_SOURCE */

#endif
