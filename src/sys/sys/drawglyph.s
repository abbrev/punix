/*
 * drawglyph.s, draw a 4x6 character glyph
 * Copyright 2004, 2007, 2008 Christopher Williams
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

.include "lcd.inc"

.section _st1, "x"

.global drawglyph
| 6x4 (small) glyph
| drawglyph(struct glyph *glyph, int row, int col, void *lcd)
drawglyph:
	movem.l	%d3/%d4,-(%sp)
	move.l	8+4(%sp),%a1	| glyph
	move	8+8(%sp),%d1	| row
	move	8+10(%sp),%d2	| col
	move.l	8+12(%sp),%a0	| lcd
	
	| get address on screen
	mulu	#6*LCD_INCY,%d1
	
	move.b	#0xf0,%d3	| AND mask for glyph
	moveq	#0x0f,%d4	| AND mask for screen
	asr	#1,%d2		| col / 2
	bcc	0f		| is col even?
	exg	%d3,%d4
0:
	add	%d2,%d1		| + col
	add	%d1,%a0
	|lea	LCD_MEM(%a0),%a0	| address on screen
	| %a0 holds screen address
	| %d3.b holds glyph mask
	| %d4.b holds screen mask
	| %a1 holds glyph address
	
	moveq	#LCD_INCY,%d0
	moveq	#6-1,%d2	| glyph height - 1
0:
	move.b	(%a1)+,%d1	| get a row from the glyph
	and.b	%d3,%d1		| mask glyph
	
	and.b	%d4,(%a0)	| mask byte on screen
	eor.b	%d1,(%a0)	| and put glyph row there
	
	| FIXME: for greyscale, change the line below to do the same as above,
	| except write to the light plane instead of the dark plane.
	move.b	(%a1)+,%d1	| get a row of glyph
	cmp.l	#0x5b00,%a0	| LCD_MEM is less than 0x5b00
	blt	1f

	and.b	%d3,%d1		| mask glyph
	
	and.b	%d4,(3840,%a0)	| mask byte on screen
	eor.b	%d1,(3840,%a0)	| and put glyph row there
	
1:	
	add	%d0,%a0		| next screen row
	dbra	%d2,0b
	
	movem.l	(%sp)+,%d3/%d4
	rts

.global drawglyphinv
| 6x4 (small) glyph, inverted
| drawglyphinv(struct glyph *glyph, int row, int col, void *lcd)
drawglyphinv:
	movem.l	%d3/%d4,-(%sp)
	move.l	8+4(%sp),%a1	| glyph
	move	8+8(%sp),%d1	| row
	move	8+10(%sp),%d2	| col
	move.l	8+12(%sp),%a0	| lcd
	
	| get address on screen
	mulu	#6*LCD_INCY,%d1
	
	move.b	#0xf0,%d3	| AND mask for glyph and the OR mask for screen
	asr	#1,%d2		| col / 2
	bcc	0f		| is col even?
	moveq	#0x0f,%d3
0:
	add	%d2,%d1		| + col
	add	%d1,%a0
	|lea	LCD_MEM(%a0),%a0	| address on screen
	| %a0 holds screen address
	| %d3.b holds glyph mask and screen mask
	| %a1 holds glyph address
	
	moveq	#LCD_INCY,%d0
	moveq	#6-1,%d2	| glyph height - 1
0:
	move.b	(%a1)+,%d1	| get a row from the glyph
	and.b	%d3,%d1		| mask glyph
	
	or.b	%d3,(%a0)	| mask byte on screen
	eor.b	%d1,(%a0)	| and put glyph row there
	
	| FIXME: for greyscale, change the line below to do the same as above,
	| except write to the light plane instead of the dark plane.
	move.b	(%a1)+,%d1	| get a row of glyph
	cmp.l	#0x5b00,%a0	| LCD_MEM is less than 0x5b00
	blt	1f

	and.b	%d3,%d1		| mask glyph
	
	or.b	%d3,(3840,%a0)	| mask byte on screen
	eor.b	%d1,(3840,%a0)	| and put glyph row there
	
1:	
	add	%d0,%a0		| next screen row
	dbra	%d2,0b
	
	movem.l	(%sp)+,%d3/%d4
	rts

.global xorcursor
| xorcursor(int row, int col, void *lcd)
| XXX: this can probably be optimized
xorcursor:
	move	4(%sp),%d0	| row
	move	6(%sp),%d1	| col
	move.l	8(%sp),%a0
	
	| get address on screen
	mulu	#6*LCD_INCY,%d0
	
	move.b	#0xf0,%d2	| XOR mask XXX
	asr	#1,%d1		| col / 2
	bcc	0f		| is col even?
	move.b	#0x0f,%d2	| XXX
0:
	add	%d1,%d0		| + col
	add	%d0,%a0
	|lea	LCD_MEM(%a0),%a0	| address on screen
	| %a0 holds screen address
	| %d2 holds cursor mask
	
	moveq	#LCD_INCY,%d0
	.rept 6
	eor.b	%d2,(%a0)	| XOR the cursor to the screen
	add	%d0,%a0		| next screen row
	.endr
	
	rts
