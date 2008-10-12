#include "punix.h"
#include "globals.h"

#define TRIGLEVEL (*(short *)0x600018)
#define BATTCHECK (*(char *)0x70001d)
#define BATTSTAT  (*(volatile char *)0x600000)
#define WAITSTATE (*(char *)0x600003)

static void reinit()
{
	int i;
	TRIGLEVEL = 0x380;
	i = 0x52;
	do {
		if (!(BATTSTAT & (1 << 2))) break;
	} while (i--);
}

int batt_check()
{
	int x = spl5();
	int i, j;
	char k;
	BATTCHECK |= 0x09;
	
	/* XXX: why 7 times? */
	i = 6;
	do {
		reinit();
		TRIGLEVEL = i << 7;
		j = 0x6e;
		do {
			k = BATTSTAT & (1 << 2);
			if (!k) goto quit;
		} while (j--);
	} while (i--);
quit:
	i = (6 - i) >> 1;
	G.batt_level = i;
	BATTCHECK &= 0xf7;
	reinit();
	BATTCHECK &= 0xf6;
	WAITSTATE = ~0;
	splx(x);
	return G.batt_level;
}
