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

#define F_SHIFT 12
#define F_ONE (1 << F_SHIFT)

STARTUP(void loadav(int numrun))
{
	static const unsigned long cexp[3] = {
		3769, 4028, 4073 /* 4096 * exp(-x/60) */
	};
	int i;
	unsigned long n = (unsigned long)numrun << F_SHIFT;

	for (i = 0; i < 3; ++i)
		G.loadavg[i] = cexp[i] * G.loadavg[i] + (F_ONE - cexp[i]) * n;
}
