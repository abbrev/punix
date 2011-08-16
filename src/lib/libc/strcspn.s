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

/*
size_t strcspn(const char *s1, const char *reject)
{
	const char *c;
	const char *s;
	for (s = s1; *s; ++s) {
		c = reject;
		while (*c)
			if (*s == *c++) goto out;
	}
out:	return s - s1;
}
*/

.global strcspn
| size_t strcspn(const char *s1, const char *reject);
strcspn:
	move.l	4(%sp),%a0	| s1
	move.l	8(%sp),%d1	| reject
	
	bra	1f
0:		move.l	%d1,%a1		| c = reject
		bra	3f
2:			cmp.b	%d2,%d0
			beq	5f		| *s != *c++?
3:			move.b	(%a1)+,%d2
			bne	2b		| *c == '\0'?
4:		addq.l	#1,%a0		| ++s
1:		move.b	(%a0),%d0
		bne	0b		| *s == '\0'?
5:	move.l	%a0,%d0
	sub.l	4(%sp),%d0	| s - s1
	rts
