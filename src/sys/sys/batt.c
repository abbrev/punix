#include "punix.h"
#include "globals.h"

#define BATTSTAT  (*(volatile char *)0x600000)
#define WAITSTATE (*(char *)0x600003)
#define TRIGLEVEL (*(volatile short *)0x600018)
#define BATTCHECK (*(char *)0x70001d)

/*
 * from j89hw.txt:
 * $600000 RW ($00)
 *  :2     1: Battery voltage level is *above* the trig level.
 *         see $600018).
 * $600018 RW ($0380)
 *  :15-10 -
 *  :9-7   Battery voltage trigger level (see $600000:2)
 *         (HW2: must also enable batt checker at $70001D:3,0)
 *          %000: ~4.6V, %001: ~4.5V, %010: ~4.3V, %011: ~4.0V,
 *          %100: ~3.7V, %101: ~3.4V, %110: ~3.2V, (%111: ~2.9V  calc resetted!)
 *         Remember to wait a while (~250us) before reading from
 *         $600000:2 to make sure that the hardware has stabilized.
 *         Keep the trig level set to the lowest voltage (%111) when
 *         its not in use.  (The keyboard does not work otherwise.)
 *         AMS displays "BATT" when the voltage drops below 4.3V and
 *         4.0V, respectively.
 */

static void reinit()
{
	int i;
	TRIGLEVEL = 0x380;
	for (i = 52; i >= 0; --i) {
		if (!(BATTSTAT & 0x04)) break;
	};
}

/* batt_check() returns one of the following values:
 * 7: >4.6V  full
 * 6: >4.5V  ok
 * 5: >4.3V  medium
 * 4: >4.0V  low
 * 3: >3.7V  very low
 * 2: >3.4V  starving
 * 1: >3.2V  almost dead
 * 0: <=3.2V dead (calculator is reset?)
 */
int batt_check()
{
	int x = spl1();
	int i, j;
	BATTCHECK |= 0x09;
	
	for (i = 6; i >= 0; --i) {
		reinit();
		TRIGLEVEL = i << 7;
		for (j = 110; j >= 0; --j) {
			if (!(BATTSTAT & 0x04))
				goto quit;
		};
	};
quit:
	i = 6 - i;
	G.batt_level = i;
	BATTCHECK &= 0xf7;
	reinit();
	BATTCHECK &= 0xf6;
	WAITSTATE = ~0;
	
	showstatus();
	splx(x);
	return G.batt_level;
}

/* timeout routine to check battery periodically */
#define BATT_TICKS (5*HZ)
void checkbatt(void *unused)
{
	batt_check();
	timeout(checkbatt, NULL, BATT_TICKS);
}

void battinit()
{
	checkbatt(NULL);
}
