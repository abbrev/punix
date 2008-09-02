#include <stdio.h>
#include <stdlib.h>

#include "sched.h"

#define DEBUG2 fprintf(stderr, "%s:%s (%d)\n", __FILE__, __FUNCTION__, __LINE__)
#define DEBUG() DEBUG2

struct proc proc[NPROC];

int runrun;
int cputime;
int ticks = 0;
int isclock = 0;
unsigned long avg[3] = {0,0,0};

static int setpri(struct proc *p)
{
	int pri;
	pri = p->p_basepri + 64 * ((p->p_nice + 20) * NICEPRIWEIGHT + p->p_cputime * CPUPRIWEIGHT) / CPUMAX;
	DEBUG();
	if (pri > 255)
		pri = 255;
	DEBUG();
	if (pri < current->p_pri)
		++runrun;
	p->p_pri = pri;
	DEBUG();
	return pri;
}

static void slp(int chan, int basepri)
{
	current->p_waitfor = chan;
	current->p_status = PSLEEP;
	current->p_basepri = basepri;
}

static void wakeup(int chan)
{
	struct proc *p;
	for EACHPROC(p) {
		if (p->p_status == PSLEEP && p->p_waitfor == chan) {
			p->p_status = PRUN;
			setpri(p);
		}
		
		if (p->p_pri < current->p_pri)
			++runrun;
	}
}

#define FSHIFT 12
#define FIXED_1 (1 << FSHIFT)

static void loadav(unsigned long *avg, int numrunning)
{
	static const unsigned long exp[3] = {
#if 0
		0353,   /* 256 * exp(-5/60)  */
		0373,   /* 256 * exp(-5/300)  */
		0376,   /* 256 * exp(-5/900) */
#else
		3769, 4028, 4073, /* 4096 * exp(...) */
#endif
	};
	
	int i;
	unsigned long n = numrunning << FSHIFT;
	printf("load average: ");
	for (i = 0; i < 3; ++i) {
		long x;
#if 0
		avg[i] = ( exp[i]*(avg[i]-(n<<16)) + (((long)n)<<(16+8)) ) >> 16;
#else
		avg[i] = (avg[i] * exp[i] + n * (FIXED_1 - exp[i])) >> FSHIFT;
#endif
		x = avg[i];
		x += FIXED_1 / 100 / 2; /* rounding */
		printf("%3d.%02d (%08lx) ", x >> FSHIFT, ((x % FIXED_1) * 100) >> FSHIFT, avg[i]);
	}
	printf("\n");
	int tot = 0;
	for (i = 0; i < NPROC; ++i) {
		if (proc[i].p_status == PFREE)
			continue;
		tot += proc[i].p_usage;
	}
	printf("cpu usage: ");
	for (i = 0; i < NPROC; ++i) {
		printf("%3d.%01d%% ", 100 * proc[i].p_usage / tot, (1000 * proc[i].p_usage / tot) % 10); 
	}
	printf("\n");
}

static int hardclock()
{
	struct proc *p;
	++ticks;
	if ((ticks%LOADFREQ) == 0) {
		int n = 0;
		for EACHPROC(p) {
			if (p->p_status == PRUN)
				++n;
		}
		loadav(avg, n);
	}
	
	cputime += 2;
	if (cputime >= 2 * QUANTUM) {
		isclock = 1;
		++runrun;
	}
	int sec = ticks / HZ;
	int min = sec / 60;
	sec %= 60;
	int hour = min / 60;
	min %= 60;
	printf("hardclock(): ticks=%d (%02d:%02d:%02d.%03d) cputime=%d\n", ticks, hour, min, sec, (ticks % HZ) * 1000 / HZ, cputime);
	return runrun;
}

static void decaycputimes()
{
	struct proc *p;
	printf("decaycputimes()\n");
	for EACHPROC(p) {
		if (p->p_status != PFREE) {
			DEBUG();
#if 1
			p->p_cputime = p->p_cputime * CPUDECAY / CPUSCALE;
#else
			p->p_cputime -= CPUDECAY;
#endif
			if (p->p_cputime < 0)
				p->p_cputime = 0;
			setpri(p);
		}
	}
}

static void cpuidle()
{
	hardclock();
}

static struct proc *nextready()
{
	struct proc *p;
	struct proc *bestp = NULL;
	int bestpri = 255;
	int minnice = 20;
	
	do {
		for EACHPROC(p) {
			if (p->p_status != PRUN)
				continue;
			if (p->p_nice >= minnice + 20)
				continue;
			if (p->p_nice < minnice)
				minnice = p->p_nice;
			if (p->p_pri < bestpri) {
				bestpri = p->p_pri;
				bestp = p;
			}
		}
	} while (bestp && bestp->p_nice >= minnice + 20);
	
	return bestp;
}

static void swtch()
{
	struct proc *p;
	int t;
	t = 0;
	if (!isclock) {
		++cputime;
		--t;
	}
	DEBUG();
	current->p_cputime += cputime * CPUSCALE / 2;
	current->p_usage += cputime;
	current->p_basepri = PUSER;
	setpri(current);
	while (current->p_cputime >= CPUMAX)
		decaycputimes();
	DEBUG();
		
	while (!(p = nextready())) {
		struct proc *pp = current;
		current = NULL;
		cpuidle();
		current = pp;
		t = 0;
	}
	DEBUG();
		
	cputime = t;
	runrun = 0;
	isclock = 0;
	
	current = p;
}

static int proc0()
{
	fprintf(stderr, "proc0()\n");
	if (cputime >= 2 * QUANTUM - 4)
		wakeup(0);
	return 0;
}

static int proc1()
{
	fprintf(stderr, "proc1()\n");
	if (cputime >= 2) {
		slp(0, PUSER);
		return 1;
	}
	return 0;
}

static int proc2()
{
	fprintf(stderr, "proc2()\n");
	if (0 && cputime >= 6) {
		slp(0, PUSER);
		return 1;
	}
	return 0;
}

static int procnone()
{
	fprintf(stderr, "procnone()\n");
	exit(-1);
	return 0;
}

int main()
{
	struct proc *p;
	int t;
	int numcalls = 0;
	
	cputime = 0;
	runrun = 0;
	
	proc[0] = (struct proc){
		PRUN, 0, 0, 0, 0, 0, PUSER, proc0
	};
	proc[1] = (struct proc){
		PRUN, 0, 20, 0, 0, 0, PUSER, proc1
	};
	proc[2] = (struct proc){
		PRUN, 0, 10, 0, 0, 0, PUSER, proc2
	};
	proc[3] = (struct proc){
		PFREE, 0, 0, 0, 0, 0, PUSER, procnone
	};
	
	current = &proc[0];
	
	DEBUG();
	decaycputimes();
	DEBUG();
	runrun = 0;
	int v;
	for (;;) {
		DEBUG();
		fprintf(stderr, "current = &proc[%d]\n", current - &proc[0]);
		for EACHPROC(p) {
			printf("%4d %3d    ", p->p_cputime, p->p_pri);
		}
		printf("\n");
		printf("before: %d\n", cputime);
		printf("%d\n", current - &proc[0]);
		v = current->p_func();
		++numcalls;
		if (!v || numcalls >= 5) {
			v = hardclock();
			numcalls = 0;
		}
		printf("after:  %d\n", cputime);
		if (v) {
			DEBUG();
			swtch();
		}
		printf("\n");
	}
}
