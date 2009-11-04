/* new Earliest Effective Virtual Deadline First scheduler,
 * based on BFS by Con Kolivas */

#include "list.h"
#include "punix.h"
#include "globals.h"
#include "sched.h"
#include "proc.h"
#include "ticks.h"

#define PRIO_RANGE 40

#define NICE_TO_PRIO(nice) ((nice) + 20)
#define PRIO_TO_NICE(prio) ((prio) - 20)
#define PROC_NICE(procp) PRIO_TO_NICE((procp)->p_nice)

#define MS_TO_TICKS(t) (((long)(t) * HZ + 500) / 1000)

/* RR_INTERVAL is the round-robin interval in milliseconds */
#define RR_INTERVAL 500 //(1000L*16/HZ)
/* TIME_SLICE is the same as RR_INTERVAL but measured in ticks */
#define TIME_SLICE MS_TO_TICKS(RR_INTERVAL)

static int prio_deadline_offset(int prio)
{
	return ((G.prio_ratios[prio]*TIME_SLICE + F_HALF) / F_ONE) ? : 1;
}

void sched_init(void)
{
	int i;
	G.prio_ratios[0] = F_ONE;
	for (i = 1; i < PRIO_RANGE; ++i)
		G.prio_ratios[i] = (G.prio_ratios[i-1] * 11 + 5) / 10;
	INIT_LIST_HEAD(&G.runqueue);
	/* TODO: do whatever else is needed */
}

/* select the next process to run. */
static struct proc *earliest_deadline_proc(void)
{
	unsigned long dl, earliest_deadline = 0;
	struct proc *procp, *earliest_procp = NULL;
	list_for_each_entry(procp, &G.runqueue, p_runlist) {
		//kprintf(".");
		dl = procp->p_deadline;
		if (time_before(dl, G.ticks)) {
			earliest_procp = procp;
			goto out;
		}
		if (earliest_procp == NULL ||
		    time_before(dl, earliest_deadline)) {
			earliest_procp = procp;
			earliest_deadline = dl;
		}
	}
	
out:
	return earliest_procp;
}

#if 1
STARTUP(void swtch())
{
	struct proc *p;
	
	//kprintf("swtch() ");
	int x = spl1(); /* higher than 256Hz timer */
	
	/* XXX: this shows the number of times this function has been called.
 	 * It draws in the bottom-left corner of the screen.
	 */
#if 1
	++*(long *)(0x4c00+0xf00-30);
#endif
	
	if (current->p_time_slice <= 0) {
		/* process used its whole time slice */
		current->p_time_slice += TIME_SLICE;
		current->p_deadline = G.ticks + prio_deadline_offset(current->p_static_prio);
	}
	
	/*
	 * When a process switches between clock ticks, we keep track of this
	 * by adding one-half of a clock tick to its cpu time. This is based
	 * on the theory that a process will use, on average, half of a clock
	 * tick when it switches between ticks. To balance the accounting, we
	 * subtract one-half clock tick from the cpu time of the next process
	 * coming in if it doesn't get a full first clock tick. Doing this will
	 * (hopefully) more correctly account for the cpu usage of processes
	 * which always switch (sleep) before the first clock tick of their
	 * time slice. The correct solution, of course, requires using a
	 * high-precision clock that tells us exactly how much cpu time the
	 * process uses, but we don't have a high-resolution clock. :(
	 */
	
	if (!istick) {
		/* we switched between ticks, so add one-half clock tick to our
		 * cpu time */
		++current->p_cputime;
	}
	
	current->p_cputime += cputime * 2;
	
	while (!(p = earliest_deadline_proc())) {
		struct proc *pp = current;
		current = NULL; /* don't bill any process if they're all asleep */
		splx(x);
		cpuidle();
		x = spl1();
		current = pp;
		//istick = 1; /* the next process will start on a tick */
	}
	
	if (!istick) {
		/* subtract half a tick from the next proc's cpu time */
		--p->p_cputime;
	}
        cputime = 0;
	G.need_resched = 0;
	istick = 0;
	
	if (p == current)
		return;
	
	if (!P.p_fpsaved) {
		/* savefp(&P.p_fps); */
		P.p_fpsaved = 1;
	}
	
	if (setjmp(P.p_ssav))
		return; /* we get here via longjmp */
	
	current = p;
	
	longjmp(P.p_ssav, 1);
}
#endif

/* called from timer at HZ times per second */
void sched_tick(void)
{
	if (!current) return;
	if (--current->p_time_slice <= 0) {
		istick = 1;
		++G.need_resched;
	}
	++cputime;
}

#if 1
/* set the state of the process to 'state' */
static void set_state(struct proc *p, int state)
{
	if (state == p->p_status) return;
	if (state == P_RUNNING) {
		++G.numrunning;
		if (time_before(p->p_deadline, current->p_deadline)) {
			++G.need_resched;
			//istick = 0;
		}
		list_add_tail(&p->p_runlist, &G.runqueue);
	} else {
		--G.numrunning;
		list_del(&p->p_runlist);
	}
	p->p_status = state;
	/* TODO: anything else? */
}
#endif

/* put the given proc on the run queue, possibly preempting the current proc */
void sched_run(struct proc *p)
{
	//kprintf("sched_run()\n");
	if (p->p_status == P_RUNNING)
		panic("trying to run an already running process");
	if (p->p_status == P_ZOMBIE)
		panic("trying to run a zombie");
	set_state(p, P_RUNNING);
	/* TODO: anything else? */
}

/* put the given process to sleep and take it off the run queue */
void sched_sleep(struct proc *p)
{
	//kprintf("sched_sleep(%08lx)\n", p);
	set_state(p, P_SLEEPING);
}

/* stop the given process and take it off the run queue */
void sched_stop(struct proc *p)
{
	//kprintf("sched_stop(%08lx)\n", p);
	set_state(p, P_STOPPED);
}

void sched_fork(struct proc *childp)
{
	//kprintf("sched_fork(%08lx)\n", childp);
}

void sched_exec(void)
{
	//kprintf("sched_exec()\n");
}

void sched_exit(struct proc *p)
{
	//kprintf("sched_exit(%08lx)\n", p);
}

#if 0
void sched_set_scheduler(struct proc *procp, int policy)
{
}

int sched_get_scheduler(struct proc *procp)
{
}
#endif

int sched_get_priority(struct proc *procp)
{
	return procp->p_static_prio;
}

void sched_set_priority(struct proc *procp, int prio)
{
}

int sched_get_nice(struct proc *procp)
{
	return PROC_NICE(procp);
}

void sched_set_nice(struct proc *procp, int nice)
{
	int oldprio = procp->p_static_prio;
	int newprio = NICE_TO_PRIO(nice);
	if (newprio == oldprio) return;
	procp->p_static_prio = newprio;
	/* adjust the deadline for this process */
	procp->p_deadline += prio_deadline_offset(newprio) -
	                     prio_deadline_offset(oldprio);
}
