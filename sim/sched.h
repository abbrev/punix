#ifndef _SCHED_H_
#define _SCHED_H_

#define NPROC 4

#define HZ 256
#define QUANTUM (HZ/32)

#if 0
#define NICEWINDOW 40
#define CPUSCALE 64
#define CPUTHRESHOLD (CPUSCALE * HZ/4)
#define CPUDECAY 1 / 16
#define CPUPRIWEIGHT NICEWINDOW / CPUTHRESHOLD
#else
#define CPUSCALE 64
#define CPUMAX (CPUSCALE * HZ/4)
#define CPUDECAY (CPUSCALE * 13 / 16)
#define CPUPRIWEIGHT 1
#define NICEPRIWEIGHT ((CPUMAX - QUANTUM * CPUSCALE) / 20)
#endif

#define LOADFREQ (5*HZ)

#define PUSER 128

#define PFREE   0
#define PRUN    1
#define PSTOP   2
#define PSLEEP  3
#define PZOMBIE 4

struct proc {
	int p_status;
	int p_waitfor;
	
	int p_nice;
	int p_cputime;
	int p_usage;
	int p_basepri;
	int p_pri;
	
	int (*p_func)(void);
};

struct proc proc[];
struct proc *current;

#define EACHPROC(p) ((p) = &proc[0]; (p) < &proc[NPROC]; ++(p))

#endif
