#define _BSD_SOURCE
#include <sys/time.h>

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
		itp->it_value.tv_nsec += 1000000000L;
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
			itp->it_value.tv_nsec += 1000000000L;
			itp->it_value.tv_sec--;
		}
	} else {
		itp->it_value.tv_nsec = 0;
	}
	return 0;
}

STARTUP(void timespecfix(struct timespec *t1))
{
	if (t1->tv_nsec < 0) {
		t1->tv_sec--;
		t1->tv_nsec += 1000000000L;
	} else if (t1->tv_nsec >= 1000000000L) {
		t1->tv_sec++;
		t1->tv_nsec -= 1000000000L;
	}
}

STARTUP(void timespecadd(struct timespec *t1, struct timespec *t2))
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_nsec += t2->tv_nsec;
	timespecfix(t1);
}

STARTUP(void timespecsub(struct timespec *t1, struct timespec *t2))
{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_nsec -= t2->tv_nsec;
	timespecfix(t1);
}

/* internal getitimer, used by sys_getitimer and sys_setitimer */
STARTUP(static void getit(int which, struct itimerval *value))
{
	struct itimerval itv;
	struct itimerspec tv;
	int x;
	
	x = spl1();
	tv = P.p_itimer[which];
	if (which == ITIMER_REAL) {
		/* convert from absolute to relative time */
		if (timespecisset(&tv.it_value)) {
			if (timespeccmp(&tv.it_value, &walltime, <))
				timespecclear(&tv.it_value);
			else
				timespecsub(&tv.it_value, &walltime);
		}
	}
	
	splx(x);
	
	itv = *(struct itimerval *)&tv;
	itv.it_interval.tv_usec /= 1000;
	itv.it_value.tv_usec /= 1000;
	
	copyout(value, &itv, sizeof(itv));
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
	    tv->tv_nsec < 0 || tv->tv_nsec >= 1000000000L)
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
	
	x = spl1();
	/*
	 * It's possible for us to be called before the real timer actually
	 * ought to go off, such as if the timespec is too large to be 
	 * represented as ticks in a long value. If that is the case, the
	 * process is not signalled, but another timeout is set for this routine
	 * to be called again, possibly when the real timer really goes off.
	 */
	if (!timespeccmp(&tp->it_value, &walltime, <)) {
		psignal(p, SIGALRM);
		
		if (!timespecisset(&tp->it_interval)) {
			timespecclear(&tp->it_value);
			goto out;
		}
	}
	
	for (;;) {
		if (timespeccmp(&tp->it_value, &walltime, >)) {
			timeout(realitexpire, p, hzto(&tp->it_value));
			goto out;
		}
		timespecadd(&tp->it_value, &tp->it_interval);
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
	
	copyin(&aitv, ap->value, sizeof(aitv));
	
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
	if (itv.it_value.tv_nsec >= 1000000000L) {
		itv.it_value.tv_sec++;
		itv.it_value.tv_nsec -= 1000000000L;
	}
#endif
	
	x = spl1();
	
	P.p_itimer[ap->which] = itv;
	
	if (ap->which == ITIMER_REAL) {
		untimeout(realitexpire, &P);
		if (timespecisset(&itv.it_value)) {
			timespecadd(&itv.it_value, &walltime);
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
	
	P.p_error = ENOSYS;
}
