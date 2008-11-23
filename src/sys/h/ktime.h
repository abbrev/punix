/* $Id$ */

#ifndef _KTIME_H_
#define _KTIME_H_

#include <time.h>

/*
 * I think I've settled on using struct timespec for all internal time units.
 * It represents times with nanosecond resolution, so a tick (1/256 second) is
 * an integral number of nanoseconds (3,906,250 nanoseconds). It can also be
 * converted to struct timeval easily.
 */

void timespecadd(struct timespec *a, struct timespec *b, struct timespec *res);
void timespecsub(struct timespec *a, struct timespec *b, struct timespec *res);

#define timespecclear(tvp) (tvp)->tv_sec = (tvp)->tv_nsec = 0
#define timespecisset(tvp) ((tvp)->tv_sec || (tvp)->tv_nsec)
#define timespeccmp(tvp, uvp, cmp) \
        ((tvp)->tv_sec cmp (uvp)->tv_sec || \
         ((tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_nsec cmp (uvp)->tv_nsec))



#endif
