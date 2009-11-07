#ifndef _PUNIX_SCHED_H_
#define _PUNIX_SCHED_H_

#define PRIO_RANGE 40

#define MAX_RT_PRIO 100
#define ISO_PRIO           (MAX_RT_PRIO)
#define NORMAL_PRIO        (MAX_RT_PRIO + 1)
#define IDLE_PRIO          (MAX_RT_PRIO + 2)
#define PRIO_LIMIT         (IDLE_PRIO + 1)

#endif
