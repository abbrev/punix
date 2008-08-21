/*
 * lcd.h
 * Copyright 2004 Chris Williams
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

/* #include "lcdglo.h" */

#define LCD_MEM		((char *)0x4c00)
#define LCD_INCY	30

#ifdef	TI89
	#define LCD_WIDTH	160
	#define LCD_HEIGHT	100
#else	/* TI-92+ and V200 */
	#define LCD_WIDTH	240
	#define LCD_HEIGHT	128
#endif
