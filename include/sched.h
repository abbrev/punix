#ifndef _SCHED_H_
#define _SCHED_H_

#include <sys/types.h>
#include <time.h>

struct sched_param {
	int sched_priority;
};

#define SCHED_OTHER  0
#define SCHED_NORMAL SCHED_OTHER
#define SCHED_FIFO   1
#define SCHED_RR     2
#define SCHED_ISO    3
#define SCHED_IDLE   4

int sched_get_priority_max(int __policy);
int sched_get_priority_min(int __policy);

int sched_getparam(pid_t __pid, struct sched_param *__param);
int sched_setparam(pid_t __pid, const struct sched_param *__param);

int sched_getscheduler(pid_t __pid);
int sched_setscheduler(pid_t __pid, int __policy,
                       const struct sched_param *__param);

int sched_rr_get_interval(pid_t __pid, struct timespec *__interval);
int sched_yield(void);

#endif /* _SCHED_H_ */
