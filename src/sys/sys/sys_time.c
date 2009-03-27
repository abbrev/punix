#define _BSD_SOURCE
#include <sys/time.h>
#include <sys/times.h>

#include <errno.h>
#include <utime.h>

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

STARTUP(void timespecadd(struct timespec *a, struct timespec *b, struct timespec *res))
{
	res->tv_sec = a->tv_sec + b->tv_sec;
	res->tv_nsec = a->tv_nsec + b->tv_nsec;
	if (res->tv_nsec >= SECOND) {
		res->tv_nsec -= SECOND;
		++res->tv_sec;
	}
}

STARTUP(void timespecsub(struct timespec *a, struct timespec *b, struct timespec *res))
{
	res->tv_sec = a->tv_sec - b->tv_sec;
	res->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (res->tv_nsec < 0) {
		res->tv_nsec += SECOND;
		--res->tv_sec;
	}
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
			if (timespeccmp(&tv.it_value, &realtime, <))
				timespecclear(&tv.it_value);
			else
				timespecsub(&tv.it_value, &realtime, &tv.it_value);
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
	if (tv->tv_sec < 0 || tv->tv_sec > 100000000L ||
	    tv->tv_nsec < 0 || tv->tv_nsec >= SECOND)
		return EINVAL;
	if (tv->tv_sec == 0 && tv->tv_nsec != 0 && tv->tv_nsec < TICK)
		tv->tv_nsec = TICK;
	return 0;
}

long hzto(struct timespec *tv);

STARTUP(int realitexpire(void *vp))
{
	int x;
	struct proc *p = (struct proc *)vp;
	struct itimerspec *tp = &p->p_itimer[ITIMER_REAL];
	
	x = splclock();
	/*
	 * It's possible for us to be called before the real timer actually
	 * ought to go off, such as if the timespec is too large to be 
	 * represented as ticks in a long value. If that is the case, the
	 * process is not signalled, but another timeout is set for this routine
	 * to be called again, possibly when the real timer really goes off.
	 */
	if (!timespeccmp(&tp->it_value, &realtime, <)) {
		psignal(p, SIGALRM);
		
		if (!timespecisset(&tp->it_interval)) {
			timespecclear(&tp->it_value);
			goto out;
		}
	}
	
	for (;;) {
		if (timespeccmp(&tp->it_value, &realtime, >)) {
			timeout(realitexpire, p, hzto(&tp->it_value));
			goto out;
		}
		timespecadd(&tp->it_value, &tp->it_interval, &tp->it_value);
	}
out:
	splx(x);
	return 0;
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
	
	/* just return if 'value' is NULL */
	if (!ap->value)
		return;
	
	if ((P.p_error = copyin(&aitv, ap->value, sizeof(aitv))))
		return;
	
	itv.it_interval.tv_sec = aitv.it_interval.tv_sec;
	itv.it_interval.tv_nsec = 1000 * aitv.it_interval.tv_usec;
	itv.it_value.tv_sec = aitv.it_value.tv_sec;
	itv.it_value.tv_nsec = 1000 * aitv.it_value.tv_usec;
	
	if (itimerfix(&itv.it_value) || itimerfix(&itv.it_interval)) {
		P.p_error = EINVAL;
		return;
	}
	
#if 1
	/* round the timer's value up to the next whole tick so it doesn't
	 * signal earlier than it should */
	itv.it_value.tv_nsec += TICK - 1;
	itv.it_value.tv_nsec -= (itv.it_value.tv_nsec % TICK);
	if (itv.it_value.tv_nsec >= SECOND) {
		itv.it_value.tv_sec++;
		itv.it_value.tv_nsec -= SECOND;
	}
#endif
	
	x = splclock();
	
	P.p_itimer[ap->which] = itv;
	
	if (ap->which == ITIMER_REAL) {
		untimeout(realitexpire, &P);
		if (timespecisset(&itv.it_value)) {
			timespecadd(&itv.it_value, &realtime, &itv.it_value);
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
	
	if (ap->tv) {
		struct timeval tv;
		int x = splclock();
		struct timespec ts = realtime;
		splx(x);
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
}

STARTUP(void sys_settimeofday())
{
	struct tod *ap = (struct tod *)P.p_arg;
	struct timeval tv;
	struct timespec newtime, difftime;
	int x;
	
	if (!suser())
		return;
	
	if (copyin(&tv, ap->tv, sizeof(tv))) {
		P.p_error = EFAULT;
		return;
	}
	
	/* microseconds must be in the range [0,1000000) */
	if (tv.tv_usec < 0 || 1000000L <= tv.tv_usec) {
		P.p_error = EINVAL;
		return;
	}
	
	/* convert struct timeval to struct timespec */
	newtime.tv_sec = tv.tv_sec;
	newtime.tv_nsec = tv.tv_usec * 1000;
	
	x = splclock();
	
	/* find the difference between newtime and realtime
	 * and add that difference to walltime */
	timespecsub(&newtime, &realtime, &difftime);
	timespecadd(&newtime, &walltime, &walltime);
	
	/* finally, set realtime to the new time */
	realtime = newtime;
	
	splx(x);
}


STARTUP(void sys_time())
{
	struct a {
		time_t *tp;
	} *ap = (struct a *)P.p_arg;
	time_t sec;
	
	splclock();
	sec = realtime.tv_sec;
	spl0();
	
	if (ap->tp)
		P.p_error = copyout(ap->tp, &sec, sizeof(sec));
	P.p_retval = sec;
}

