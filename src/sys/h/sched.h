#ifndef _PUNIX_SCHED_H_
#define _PUNIX_SCHED_H_

#include <sched.h>

#define PRIO_RANGE 40

#define MAX_RT_PRIO 100
#define ISO_PRIO           (MAX_RT_PRIO)
#define NORMAL_PRIO        (MAX_RT_PRIO + 1)
#define IDLE_PRIO          (MAX_RT_PRIO + 2)
#define PRIO_LIMIT         (IDLE_PRIO + 1)

void sched_init(void);
void swtch();
void sched_tick(void);
void sched_run(struct proc *p);
void sched_sleep(struct proc *p);
void sched_stop(struct proc *p);
void sched_fork(struct proc *childp);
void sched_exec(void);
void sched_exit(struct proc *p);
#if 0
void sched_set_scheduler(struct proc *procp, int policy);
int sched_get_scheduler(struct proc *procp);
#endif
int sched_get_nice(const struct proc *p);
void sched_set_nice(struct proc *p, int newnice);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_getscheduler(pid_t pid);
int sched_yield(void);

#endif
