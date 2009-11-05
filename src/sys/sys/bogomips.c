/* bogomips.c, from init/calibrate.c in Linux 2.6
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * Adapted for Punix by Christopher Williams
 */

#include "punix.h"
#include "param.h"
#include "globals.h"

#if 0
/* portable delay */
void delay(volatile unsigned long loops)
{
	while (--loops)
		;
}
#endif

void bogomips(void)
{
	unsigned long loops_per_tick = 1;
	
	unsigned long ticks, loopbit;

	kprintf("Calibrating delay loop... ");
	while ((loops_per_tick <<= 1) != 0) {
		/* wait for "start of" clock tick */
		ticks = G.ticks;
		while (ticks == G.ticks)
			/* nothing */;
		/* Go ... */
		ticks = G.ticks;
		delay(loops_per_tick);
		ticks = G.ticks - ticks;
		if (ticks)
			break;
	}

	/*
	* Do a binary approximation to get loops_per_tick set to
	* equal one clock
	*/
	loops_per_tick >>= 1;
	loopbit = loops_per_tick;
	while ((loopbit >>= 1)) {
		loops_per_tick |= loopbit;
		ticks = G.ticks;
		while (ticks == G.ticks)
			/* nothing */;
		ticks = G.ticks;
		delay(loops_per_tick);
		if (G.ticks != ticks)   /* longer than 1 tick */
			loops_per_tick &= ~loopbit;
	}
	kprintf("%lu.%02lu BogoMips (lpt=%lu)\n",
	        loops_per_tick*HZ/500000,
	        (loops_per_tick*HZ/50) % 10000,
	        loops_per_tick);

}
