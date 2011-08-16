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

.section .text

.global memchr, memchr_reg
| void *memchr(const void *s, int c, size_t n); 
memchr:
	move.l	4(%a7),%a0	| s
	move.b	9(%a7),%d2	| c
	move.l	10(%a7),%d0	| n
memchr_reg:
	beq	9f
	bra	1f
0:		cmp.b	(%a0)+,%d2
1:		subq.l	#1,%d0
		bne	0b
	subq.l	#1,%a0
	beq	1f
9:	suba.l	%a0,%a0
1:	rts
