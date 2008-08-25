#include "punix.h"
#include "lcd.h"
#include "proc.h"
#include "buf.h"
#include "dev.h"
#include "filsys.h"
#include "inode.h"
#include "mount.h"
#include "queue.h"
#include "globals.h"

/*
 * Bit Usage
 *  7 HW1: Voltage multiplier enable. Keep set (=1).
 *  4 HW1: Screen disable (power down).
 *    HW2: LCD contrast bit 4 (MSb).
 *  3-0 LCD contrast bits 3-0 (bit 3 is MSb on HW1).
 *
 */

/* input: cont = contrast level between 0 and CONTRASTMAX, inclusive
 *        higher values => darker screen
 */
/* FIXME: this should be improved */
int lcd_set_contrast(int cont)
{
	int ch;
	if (cont < 0)
		cont = 0;
	if (CONTRASTMAX < cont)
		cont = CONTRASTMAX;
#ifdef TI89
	ch = 0x80 | cont;
#else
	ch = 0x80 | ~cont;
#endif
	CONTRASTPORT = ch;
	G.contrast = cont;
	return cont;
}

int lcd_inc_contrast()
{
	return lcd_set_contrast(G.contrast+1);
}

int lcd_dec_contrast()
{
	return lcd_set_contrast(G.contrast-1);
}

void lcdinit()
{
	lcd_set_contrast(CONTRASTMAX/2+1);
}
