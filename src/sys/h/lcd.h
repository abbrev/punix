/*
 * lcd.h
 * Copyright 2004, 2009 Chris Williams
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LCD_H_
#define _LCD_H_

#define LCD_MEM		((volatile char *)0x4c00)
#define LCD_INCY	30

#define CONTRASTMAX	31
#define CONTRASTPORT	(*(volatile char *)0x60001d)
#define CONTRAST_VMUL	0100
#define CONTRAST_FIELD	0017

/*
 * Bit Usage on 0x60001d
 *  7 HW1: Voltage multiplier enable. Keep set (=1).
 *  4 HW1: Screen disable (power down).
 *    HW2: LCD contrast bit 4 (MSb).
 *  3-0 LCD contrast bits 3-0 (bit 3 is MSb on HW1).
 *
 */

#if CALC_HAS_LARGE_SCREEN
	#define LCD_WIDTH	240
	#define LCD_HEIGHT	128
#else
	#define LCD_WIDTH	160
	#define LCD_HEIGHT	100
#endif

void lcdinit();
int lcd_set_contrast(int cont);
int lcd_inc_contrast();
int lcd_dec_contrast();

#endif
