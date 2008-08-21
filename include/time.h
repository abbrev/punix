#ifndef _TIME_H_
#define _TIME_H_

/* $Id: time.h,v 1.3 2008/01/11 13:33:02 fredfoobar Exp $ */

#include <sys/types.h>
/*#include <stddef.h>*/

#if 0
typedef int clockid_t; /* used in clock and timer functions */
typedef int timer_t;   /* returned by "timer_create". (?) */
#endif

struct tm {
	int tm_sec;   /* seconds [0,60] */
	int tm_min;   /* minutes [0,59] */
	int tm_hour;  /* hour [0,23] */
	int tm_mday;  /* day of month [1,31] */
	int tm_mon;   /* month of year [0,11] */
	int tm_year;  /* years since 1900 */
	int tm_wday;  /* day of week [0,6] (Sunday = 0) */
	int tm_yday;  /* day of year [0,365] */
	int tm_isdst; /* Daylight Savings flag */
};

/* [XSI] [x> */
#define CLOCKS_PER_SEC	1000000L
/* <x] */

/* [TMR] [x> */
struct timespec {
	time_t tv_sec;  /* seconds */
	long   tv_nsec; /* nanoseconds */
};

struct itimerspec {
	struct timespec it_interval; /* timer period */
	struct timespec it_value;    /* timer expiration */
};
/* <x] */

#if 0
/* FIXME: define these */
#define CLOCK_REALTIME
#define TIMER_ABSTIME
#define CLOCK_MONOTONIC
#endif

struct timezone {
	int tz_minuteswest; /* minutes west of Greenwich */
	int tz_dsttime;     /* type of dst correction */
};

extern char *asctime(const struct tm *__tm);
extern clock_t clock(void);
extern char *ctime(const time_t *__timep);
extern double difftime(time_t __time1, time_t __time2);
extern struct tm *gmtime(const time_t *__timep);
extern struct tm *localtime(const time_t *__timep);
extern time_t mktime(struct tm *__tm);
extern size_t strftime(char *__s, size_t __max, const char *__format,
                       const struct tm *__tm);
extern char *strptime(const char *__s, const char *__format, struct tm *__tm);
extern time_t time(time_t *__timep);
extern void tzset(void);

extern int daylight;
extern long timezone;
extern char *tzname[];

#if 0
#define __isleap(year)	\
	((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
extern void __tm_conv(struct tm *tmbuf, time_t *t, int offset);
extern void __asctime(char *, struct tm *);

extern unsigned long convtime(time_t *time_field);
#endif

#endif /* _TIME_H_ */
