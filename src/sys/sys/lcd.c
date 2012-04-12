#include "punix.h"
#include "lcd.h"
#include "globals.h"

/* input: cont = contrast level between 0 and CONTRASTMAX, inclusive
 *        higher values => darker screen
 */
/* FIXME: this should be improved */
STARTUP(int lcd_set_contrast(int cont))
{
	int ch;
	if (cont < 0)
		cont = 0;
	else if (CONTRASTMAX < cont)
		cont = CONTRASTMAX;
#if CALC_HAS_LARGE_SCREEN
	ch = CONTRAST_VMUL | ~cont;
#else
	ch = CONTRAST_VMUL | cont;
#endif
	CONTRASTPORT = ch;
	G.contrast = cont;
	return cont;
}

STARTUP(int lcd_inc_contrast())
{
	return lcd_set_contrast(G.contrast+1);
}

STARTUP(int lcd_dec_contrast())
{
	return lcd_set_contrast(G.contrast-1);
}

STARTUP(int lcd_reset_contrast())
{
	return lcd_set_contrast(G.contrast);
}

STARTUP(void lcdinit())
{
	lcd_set_contrast(CONTRASTMAX/2);
}
