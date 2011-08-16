/*
 * PedroM - Operating System for Ti-89/Ti-92+/V200.
 * Copyright (C) 2003 PpHd
 * 
 * $Id: string.s 456 2011-08-15 20:11:16Z fredfoobar $
 * 
 * 2005-04-05 Chris Williams
 *   changed routines to work with `size_t' (as the standards dictate), where
 *   `size_t' is 32 bits wide (two words)
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

| Memory move / copy
| String functions

.section text

.global strncmp
| int strncmp(const char *s1, const char *s2, size_t n);
strncmp:
	move.l	4(%sp),%a0	| s1
	move.l	8(%sp),%a1	| s2
	move.l	12(%sp),%d1	| n
strncmp_reg:
	clr	%d0

	tst.l	%d1
	bne	1f
	rts
0:	sub.b	(%a1)+,%d0
	bne	0f		| same?
	subq	#1,%d1
	beq	0f		| n != 0?
1:	move.b	(%a0)+,%d0
	bne	0b		| '\0'?
	sub.b	(%a1),%d0
0:	ext.w	%d0
	rts
