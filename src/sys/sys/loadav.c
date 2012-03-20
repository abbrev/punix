/*
 * Compute Tenex style load average.  This code is adapted from similar code
 * by Bill Joy on the Vax system.  The major change is that we avoid floating
 * point since not all pdp-11's have it.  This makes the code quite hard to
 * read - it was derived with some algebra.
 *
 * "floating point" numbers here are stored in a 16 bit short, with 8 bits on
 * each side of the decimal point.  Some partial products will have 16 bits to
 * the right.
 *
 * The Vax algorithm is:
 *
 * / *
 *  * Constants for averages over 1, 5, and 15 minutes
 *  * when sampling at 5 second intervals.
 *  * /
 * double	cexp[3] = {
 * 	0.9200444146293232,	/ * exp(-1/12) * /
 * 	0.9834714538216174,	/ * exp(-1/60) * /
 * 	0.9944598480048967,	/ * exp(-1/180) * /
 * };
 * 
 * / *
 *  * Compute a tenex style load average of a quantity on
 *  * 1, 5 and 15 minute intervals.
 *  * /
 * loadav(avg, n)
 * 	register double *avg;
 * 	int n;
 * {
 * 	register int i;
 * 
 * 	for (i = 0; i < 3; i++)
 * 		avg[i] = cexp[i] * avg[i] + n * (1.0 - cexp[i]);
 * }
 */

#include "punix.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* numrun is in fixed point */
void loadav(unsigned long numrun)
{
	static const unsigned long cexp[3] = {
		EXP_1, EXP_5, EXP_15
	};
	int i;

	for (i = 0; i < 3; ++i) {
		G.loadavg[i] = ((cexp[i] * G.loadavg[i]
		                 + (F_ONE - cexp[i]) * numrun)
		                 /*+ (F_ONE / 2)*/) >> F_SHIFT;
	}
}

void calcloadav(void *unused)
{
#if 0
	/* sanity check */
	struct proc *p;
	int i;
	int n = 0;
	int x = spl7();
	for (i = 0; i < PRIO_LIMIT; ++i)
		list_for_each_entry(p, &G.runqueues[i], p_runlist)
			++n;
	if (G.numrunning != n) {
		kprintf("warning: numrunning=%d n=%d\n",
			G.numrunning, n);
		G.numrunning = n;
	}
	splx(x);
#endif
	loadav((unsigned long)G.cumulrunning * F_ONE / (HZ * 5));
	
	G.cumulrunning = 0;
#if 0
	++*(long *)(0x4c00+0xf00-26);
#endif
	timeout(calcloadav, NULL, HZ * 5);
}

void loadavinit()
{
	calcloadav(NULL);
}
