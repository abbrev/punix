#define _BSD_SOURCE
#include <sys/time.h>
#include <sys/times.h>

#include <assert.h>
#include <errno.h>
#include <utime.h>
#include <stdlib.h>

#include "punix.h"
#include "param.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "callout.h"
#include "time.h"
#include "globals.h"

struct itimera {
	int which;
	struct itimerval *value;
	struct itimerval *ovalue;
};

STARTUP(int itimerdecr(struct itimerspec *itp, long nsec))
{
	itp->it_value.tv_nsec -= nsec;
	if (itp->it_value.tv_nsec < 0) {
		if (itp->it_value.tv_sec == 0) {
			nsec = itp->it_value.tv_nsec;
			goto expire;
		}
		itp->it_value.tv_nsec += SECOND;
		itp->it_value.tv_sec--;
	}
	if (timespecisset(&itp->it_value))
		return 1;
	nsec = 0;
expire:
	if (timespecisset(&itp->it_interval)) {
		itp->it_value = itp->it_interval;
		itp->it_value.tv_nsec += nsec;
		if (itp->it_value.tv_nsec < 0) {
			itp->it_value.tv_nsec += SECOND;
			itp->it_value.tv_sec--;
		}
	} else {
		itp->it_value.tv_nsec = 0;
	}
	return 0;
}

static inline void normalize_timespec(struct timespec *a)
{
	while (a->tv_nsec < 0) {
		a->tv_nsec += SECOND;
		--a->tv_sec;
	}
	while (a->tv_nsec >= SECOND) {
		a->tv_nsec -= SECOND;
		++a->tv_sec;
	}
}

STARTUP(void timespecadd(struct timespec *a, struct timespec *b, struct timespec *res))
{
	res->tv_sec = a->tv_sec + b->tv_sec;
	res->tv_nsec = a->tv_nsec + b->tv_nsec;
	normalize_timespec(res);
}

STARTUP(void timespecsub(struct timespec *a, struct timespec *b, struct timespec *res))
{
	res->tv_sec = a->tv_sec - b->tv_sec;
	res->tv_nsec = a->tv_nsec - b->tv_nsec;
	normalize_timespec(res);
}

STARTUP(void timeradd(struct timeval *a, struct timeval *b, struct timeval *res))
{
	res->tv_sec = a->tv_sec + b->tv_sec;
	res->tv_usec = a->tv_usec + b->tv_usec;
	if (res->tv_usec >= 1000000L) {
		res->tv_usec -= 1000000L;
		++res->tv_sec;
	}
}

STARTUP(void timersub(struct timeval *a, struct timeval *b, struct timeval *res))
{
	res->tv_sec = a->tv_sec - b->tv_sec;
	res->tv_usec = a->tv_usec - b->tv_usec;
	if (res->tv_usec < 0) {
		res->tv_usec += 1000000L;
		--res->tv_sec;
	}
}

/*
 * getrealtime() returns the real time as struct timespec. If getrealtime() is
 * called more than once per clock tick, it increments the time that it returns
 * by RTINCR to ensure that time increases monotonically. RTINCR is less than
 * or equal to the length of time, in nanoseconds, that a single call to
 * getrealtime() takes.
 */
/* 12MHz ~= 83 ns per cpu clock */
#define RTINCR (100 * 83) /* ns */
void getrealtime(struct timespec *tsp)
{
	struct timespec rt;
	struct timespec offset;
	int t;
	
	int x = splclock();
	rt = realtime;
	t = G.ticks;
	
	if (t == G.rt.lasttime) {
		G.rt.incr += RTINCR;
	} else {
		G.rt.incr = 0;
		G.rt.lasttime = t;
	}
	offset.tv_sec = 0;
	offset.tv_nsec = G.rt.incr;
	splx(x);
	
	timespecadd(&rt, &offset, tsp);
}

/* setrealtime() sets the real time to the given timespec */
void setrealtime(struct timespec *ts)
{
	int x = splclock();
	realtime = *ts;
	splx(x);
}

/* internal getitimer, used by sys_getitimer and sys_setitimer */
STARTUP(static void getit(int which, struct itimerval *value))
{
	struct itimerval itv;
	struct itimerspec tv;
	int x;
	
	x = splclock();
	tv = P.p_itimer[which];
	if (which == ITIMER_REAL) {
		/* convert from absolute to relative time */
		if (timespecisset(&tv.it_value)) {
			struct timespec rt = realtime_mono;
			if (timespeccmp(&tv.it_value, &rt, <))
				timespecclear(&tv.it_value);
			else
				timespecsub(&tv.it_value, &rt, &tv.it_value);
		}
	}
	
	splx(x);
	
	itv.it_interval.tv_sec = tv.it_interval.tv_sec;
	itv.it_interval.tv_usec = tv.it_interval.tv_nsec / 1000;
	itv.it_value.tv_sec = tv.it_value.tv_sec;
	itv.it_value.tv_usec = tv.it_value.tv_nsec / 1000;
	
	P.p_error = copyout(value, &itv, sizeof(itv));
}

/* int getitimer(int which, struct itimerval *value); */
STARTUP(void sys_getitimer())
{
	struct itimera *ap = (struct itimera *)P.p_arg;
	
	if ((unsigned)ap->which > 2) {
		P.p_error = EINVAL;
		return;
	}
	
	if (!ap->value) {
		P.p_error = EFAULT;
		return;
	}
	
	getit(ap->which, ap->value);
}

STARTUP(static int itimerfix(struct timespec *tv))
{
	if (tv->tv_sec < 0 || 100000000L < tv->tv_sec ||
	    tv->tv_nsec < 0 || SECOND <= tv->tv_nsec)
		return (P.p_error = EINVAL);
	if (tv->tv_sec == 0 && tv->tv_nsec != 0 && tv->tv_nsec < TICK)
		tv->tv_nsec = TICK;
	return 0;
}

STARTUP(long hzto(struct timespec *tv))
{
	long ticks;
	struct timespec diff;
	struct timespec rt;
	int x = splclock();
	
	rt = realtime_mono;
	timespecsub(tv, &rt, &diff);
	
	if (diff.tv_sec < 0)
		ticks = 0;
	else if ((diff.tv_sec + 1) <= LONG_MAX / HZ)
		ticks = diff.tv_sec * HZ + diff.tv_nsec / TICK;
	else
		ticks = LONG_MAX;
	
	splx(x);
	return ticks;
}

STARTUP(void realitexpire(void *vp))
{
	int x;
	struct proc *p = (struct proc *)vp;
	struct itimerspec *tp = &p->p_itimer[ITIMER_REAL];
	struct timespec rt;
	
	x = splclock();
	rt = realtime_mono;
	/*
	 * It's possible for us to be called before the real timer actually
	 * ought to go off, such as if the timespec is too large to be 
	 * represented as ticks in a long value. If that is the case, the
	 * process is not signalled, but another timeout is set for this routine
	 * to be called again, possibly when the real timer really goes off.
	 */
	if (!timespeccmp(&tp->it_value, &rt, >)) {
		//kprintf("SIGALRM\n");
		psignal(p, SIGALRM);
		
		if (!timespecisset(&tp->it_interval)) {
			timespecclear(&tp->it_value);
			goto out;
		}
	}
	
	for (;;) {
		if (timespeccmp(&tp->it_value, &rt, >)) {
			timeout(realitexpire, p, hzto(&tp->it_value));
			goto out;
		}
		timespecadd(&tp->it_value, &tp->it_interval, &tp->it_value);
	}
out:
	splx(x);
}

/* int setitimer(int which, const struct itimerval *value,
    struct itimerval *ovalue); */
STARTUP(void sys_setitimer())
{
	struct itimera *ap = (struct itimera *)P.p_arg;
	struct itimerval aitv;
	struct itimerspec itv;
	int x;
	
	if ((unsigned)ap->which > 2) {
		P.p_error = EINVAL;
		return;
	}
	
	if (ap->ovalue)
		getit(ap->which, ap->ovalue);
	
	if (ap->value) {
		if ((P.p_error = copyin(&aitv, ap->value, sizeof(aitv))))
			return;
		
		itv.it_interval.tv_sec = aitv.it_interval.tv_sec;
		itv.it_interval.tv_nsec = 1000 * aitv.it_interval.tv_usec;
		itv.it_value.tv_sec = aitv.it_value.tv_sec;
		itv.it_value.tv_nsec = 1000 * aitv.it_value.tv_usec;
		
		if (itimerfix(&itv.it_value) || itimerfix(&itv.it_interval)) {
			return;
		}
#if 1
		/* round the timer's value up to the next whole tick so it
		 * doesn't signal earlier than it should */
		itv.it_value.tv_nsec += TICK - 1;
		itv.it_value.tv_nsec -= (itv.it_value.tv_nsec % TICK);
		if (itv.it_value.tv_nsec >= SECOND) {
			itv.it_value.tv_sec++;
			itv.it_value.tv_nsec -= SECOND;
		}
#endif
	
	} else {
		/* treat it as a zero timer (interval=value=0) */
		memset(&itv, 0, sizeof(itv));
	}
	
	x = splclock();
	P.p_itimer[ap->which] = itv;
	
	if (ap->which == ITIMER_REAL) {
		untimeout(realitexpire, &P);
		if (timespecisset(&itv.it_value)) {
			struct timespec rt = realtime_mono;
	
			//kprintf("setitimer: setting realitexpire in %ld.%09ld seconds\n", itv.it_value.tv_sec, itv.it_value.tv_nsec);
			timespecadd(&itv.it_value, &rt, &itv.it_value);
#if 0
			kprintf("setitimer: realtime=%ld.%09ld\n", rt.tv_sec, rt.tv_nsec);
			kprintf("setitimer: expires= %ld.%09ld\n", itv.it_value.tv_sec, itv.it_value.tv_nsec);
#endif
			P.p_itimer[ap->which] = itv;
			timeout(realitexpire, &P, hzto(&itv.it_value));
		}
	}
	
	splx(x);
}

/* XXX: should this go in a different file? */
STARTUP(void sys_utime())
{
	struct a {
		const char *path;
		const struct utimebuf *times;
	} *ap = (struct a *)P.p_arg;
	
	(void)ap;
	P.p_error = ENOSYS;
}

/*
 * int gettimeofday(struct timeval *tv, struct timezone *tz);
 * int settimeofday(const struct timeval *tv , const struct timezone *tz);
 */

struct tod {
	struct timeval *tv;
	struct timezone *tz;
};

STARTUP(void sys_gettimeofday())
{
	struct tod *ap = (struct tod *)P.p_arg;
	int whereami = G.whereami;
	G.whereami = WHEREAMI_GETTIMEOFDAY;
	
	if (ap->tv) {
		struct timeval tv;
		struct timespec ts;
		getrealtime(&ts);
		tv.tv_sec = ts.tv_sec;
		tv.tv_usec = ts.tv_nsec / 1000;
		if (copyout(ap->tv, &tv, sizeof(tv)))
			P.p_error = EFAULT;
	}
	
#if 0
	if (ap->tz) {
		P.p_error = copyout(ap->tz, &tz, sizeof(tz));
	}
#endif
	G.whereami = whereami;
}

/* microseconds must be in the range [0,1000000) */
static int usec_in_range(suseconds_t usec)
{
	return 0 <= usec && usec < 1000000L;
}

STARTUP(void sys_settimeofday())
{
	struct tod *ap = (struct tod *)P.p_arg;
	struct timeval tv;
	struct timespec newtime;
	
	if (!suser())
		return;
	
	if (copyin(&tv, ap->tv, sizeof(tv))) {
		P.p_error = EFAULT;
		return;
	}
	
	if (!usec_in_range(tv.tv_usec)) {
		P.p_error = EINVAL;
		return;
	}
	
	/* convert struct timeval to struct timespec */
	newtime.tv_sec = tv.tv_sec;
	newtime.tv_nsec = tv.tv_usec * 1000;
	
	timeadj = 0; /* remove any outstanding adjustments by adjtime() */
	setrealtime(&newtime);
}

static long timeval_to_usec(struct timeval *tv)
{
	return tv->tv_sec * 1000000L + tv->tv_usec;
}

static struct timeval *usec_to_timeval(long usec, struct timeval *tv)
{
	tv->tv_sec = usec / 1000000L;
	tv->tv_usec = usec % 1000000L;
	if (tv->tv_usec < 0) {
		--tv->tv_sec;
		tv->tv_usec += 1000000L;
	}
	return tv;
}

/* 2145 seconds */
#define MAXADJ (LONG_MAX / 1000000L - 2)
#define MINADJ (LONG_MIN / 1000000L + 2)

/* int adjtime(const struct timeval *delta, struct timeval *olddelta); */
STARTUP(void sys_adjtime())
{
	struct a {
		struct timeval *delta;
		struct timeval *olddelta;
	} *ap = (struct a *)P.p_arg;
	
	struct timeval tv;
	int error = 0;
	
	if (ap->olddelta) {
		/* return the current timeadj as a struct timeval */
		usec_to_timeval(timeadj, &tv);
		if (copyout(ap->olddelta, &tv, sizeof(tv))) {
			error = EFAULT;
		}
	}
	
	if (ap->delta && suser()) {
		/* set timeadj to delta in microseconds */
		if (copyin(&tv, ap->delta, sizeof(tv))) {
			error = EFAULT;
			goto out;
		}
		if (tv.tv_sec > MAXADJ ||
		    tv.tv_sec < MINADJ ||
		    tv.tv_usec < -1000000L ||
		    tv.tv_usec >  1000000L) {
			error = EINVAL;
			goto out;
		}
		timeadj = timeval_to_usec(&tv);
	}
out:
	P.p_error = error;
}

STARTUP(void sys_time())
{
	struct a {
		time_t *tp;
	} *ap = (struct a *)P.p_arg;
	time_t sec;
	struct timespec ts;
	
	getrealtime(&ts);
	sec = ts.tv_sec;
	
	if (ap->tp)
		P.p_error = copyout(ap->tp, &sec, sizeof(sec));
	P.p_retval = sec;
}

/*
 * getloadavg1() is like getloadavg() in BSD/Linux, but this version returns
 * load averages as 16.16 fixed-point values rather than as doubles (since we
 * do not support floating-point yet).
 * This system call may be deprecated in the future when or if floating point
 * is supported.
 */
STARTUP(void sys_getloadavg1())
{
	struct a {
		long *loadavg;
		int nelem;
	} *ap = (struct a *)P.p_arg;
	
	int whereami = G.whereami;
	G.whereami = WHEREAMI_GETLOADAVG1;
	int nelem = ap->nelem;
	int i;
	long *la = ap->loadavg;
	
	if (nelem < 0) {
		P.p_error = EINVAL; /* ??? */
		goto out;
	}
	if (nelem > 3) nelem = 3;
	splclock();
	for (i = 0; i < nelem; ++i)
		la[i] = G.loadavg[i] << (16 - F_SHIFT);
	spl0();
out:
	G.whereami = whereami;
}

