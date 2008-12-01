| long.s
| Copyright (C) 2003, 2005-2008 Patrick Pelissier
| Modified for Punix by Christopher Williams
| 
| $Id$
| 
| This program is free software; you can redistribute it and/or modify it under
| the terms of the GNU General Public License as published by the Free Software
| Foundation; either version 2 of the License, or (at your option) any later
| version.
| 
| This program is distributed in the hope that it will be useful, but WITHOUT
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
| FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
| 
| You should have received a copy of the GNU General Public License along with
| this program; if not, write to the
| Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
| 02111-1307 USA

.section _st1, "x"
.global __mulsi3
.global __umodsi3, __udivsi3
.global __modsi3, __divsi3

| **************************************************************
| * Muls with long
| **************************************************************

__mulsi3:
	move.l	(4,%sp),%d1
	move.l	(8,%sp),%d2
	move.l	%d2,%d0
	mulu	%d1,%d0
	swap	%d2
	mulu	%d1,%d2
	swap	%d1
	mulu	(10,%sp),%d1
	add.w	%d1,%d2
	swap	%d2
	clr.w	%d2
	add.l	%d2,%d0
	rts

| **************************************************************
| * Divu/modu with short
| **************************************************************

| Modulo
| In:
|	%d0.w = Unsigned Int 16 bits
|	%d1.w = Signed Int 16 bits
| Out:
|	%d1.l = %d1 % %d0
_ms16u16:
	ext.l	%d1
	bpl.s	ms16u16_sup
		neg.l	%d1
		divu	%d0,%d1
		swap	%d1
_dm16end2:	neg.w	%d1
		bra.s	_dm16end
ms16u16_sup:	
	divu	%d0,%d1
	swap	%d1
_dm16end:
	ext.l	%d1
	rts

| In:
|	%d1.w = signed int
|	%d0.w = unsigned int
| Out:
|	%d1.l = %d1/%d0
_ds16u16:
	ext.l	%d1
	bpl.s	0f
		neg.l	%d1
		divu	%d0,%d1
		bra.s	_dm16end2
0:	divu	%d0,%d1
	bra.s	_dm16end

| In:
|	%d1.w = unsigned int
|	%d0.w = unsigned int
| Out:
|	%d1.l = %d1/%d0
_du16u16:
	moveq	#0,%d2
	move.w	%d1,%d2
	divu	%d0,%d2
	moveq	#0,%d1
	move.w	%d2,%d1
	rts

| In:
|	%d1.w = unsigned int
|	%d0.w = unsigned int
| Out:
|	%d1.l = %d1%%d0
_mu16u16:
	swap	%d1
	clr.w	%d1
	swap	%d1
	divu	%d0,%d1
	clr.w	%d1
	swap	%d1
	rts

| **************************************************************
| * Divu/modu with long
| **************************************************************

__umodsi3:
	move.l 4(%sp),%d1
	move.l 8(%sp),%d0
	bsr	_mu32u32
	move.l	%d1,%d0
	rts

__udivsi3:
	move.l 4(%sp),%d1
	move.l 8(%sp),%d0
	bsr	_du32u32
	move.l	%d1,%d0
	rts

| **************************************************************
| * Divs/mods with long
| **************************************************************

__modsi3:
	move.l 4(%sp),%d1
	move.l 8(%sp),%d0
	bsr	_ms32s32
	move.l	%d1,%d0
	rts

__divsi3:
	move.l 4(%sp),%d1
	move.l 8(%sp),%d0
	bsr	_ds32s32
	move.l	%d1,%d0
	rts

ms32s32_negd0:
	neg.l	%d0
	tst.l	%d1
	blt.s	ms32s32_negd1
	bra.s	_mu32u32

| In:
|	%d1.l = long
|	%d0.l = long
| Out:
|	%d1.l = %d1%%d0
| Destroy:
|	%d0-%d2/%a0-%a1
_ms32s32:
	tst.l	%d0
	beq.s	ds32s32_divby0
	blt.s	ms32s32_negd0
	tst.l	%d1
	bge.s	_mu32u32
ms32s32_negd1:
	neg.l	%d1
ms32s32_oneneg:
	bsr.s	_du32u32
	move.l	%d2,%d1
	neg.l	%d1
	rts

| Here %d0 < 0, and %d1 is ?
ds32s32_negd0:
	neg.l	%d0
	tst.l	%d1
	bgt.s	ds32s32_oneneg
	neg.l	%d1
	bra.s	_du32u32

| Here %d0 > 0 and %d1 < 0
ds32s32_negd1:
	neg.l	%d1
ds32s32_oneneg:
	bsr.s	_du32u32
	neg.l	%d1
	rts

| In:
|	%d1.l = unsigned long
|	%d0.l = unsigned long
| Out:
|	%d1.l = %d1%%d0
| Destroy:
|	%d0-%d2/%a0-%a1
_mu32u32:
	bsr.s	_du32u32
	move.l	%d2,%d1
	rts

ds32s32_divby0:	divu.w	#0,%d1

| In:
|	%d1.l = long
|	%d0.l = long
| Out:
|	%d1.l = %d1/%d0
| Destroy:
|	%d0-%d2/%a0-%a1
_ds32s32:
	tst.l	%d0
	beq.s	ds32s32_divby0
	blt.s	ds32s32_negd0
	tst.l	%d1
	blt.s	ds32s32_negd1

| In:
|	%d1.l = unsigned long
|	%d0.l = unsigned long
| Out:
|	%d1.l = %d1/%d0
|	%d2.l = %d1%%d0
| Destroy:
|	%d0-%d2/%a0-%a1
_du32u32:
	| First check if %d0 >= %d1
	cmp.l	%d1,%d0
	bcs.s	1f
		beq.s	0f
		move.l	%d1,%d2
		moveq	#0,%d1
		rts
0:		moveq	#1,%d1
		moveq	#0,%d2
		rts
1:
	| Check if HIGH 16 bits of denominator if NULL
	swap	%d0
	tst.w	%d0
	bne.s	1f
		| High 16 bits of %d0 are null
		swap	%d0
		divu	%d0,%d1
		bvs.s	0f
			| No overflow! Fantastic!
			moveq	#0,%d2
			move.w	%d1,%d2	| %d2=quotient
			clr.w	%d1	| %d1=rest
			swap	%d1
			exg	%d1,%d2	| Exg them
			rts
0:		| %d1 isn't changed
		| A.32 bits B.16 bits
		| Compute A/(2^16*B) = q1*2^16*b+r1 with 0 <= r1 < 2^16*b
		| Then r1/b is between 0 and 2^16!
		| Compute r1/b=q2*b+r2
		| Return (q1*2^16+q2, r2)
		move.w	%d1,-(%a7)		| Save low 16 bits
		clr.w	%d1			| Shift %d1 by 16
		swap	%d1			|
		divu	%d0,%d1			| Divide %d1.uw by %d0.w
		move.l	%d1,%d2			| %d2.uw = r1 shifted by 16
		swap	%d1			| Put High quotient in High part of register
		move.w	(%a7)+,%d2		| Reload remaining of A: r'=r1<<16+ra
		divu	%d0,%d2			| Rediv. %d2.uw=r2 and %d2.w=q2
		move.w	%d2,%d1			| q = q1 + q2
		clr.w	%d2			| %d2.w = 0
		swap	%d2			| %d2.uw -> %d2.w = %d2.l
		rts
1:
	| A loop. Doesn't do more than 16 steps. Classic division algorithm.
	| q = 0
	| do
	|	qq = 1<<(nbitsA-nbitsB-1)
	|	A = A - qq*B
	|	q += qq
	| while A > B
	moveq	#0,%d2		| q
	exg	%d1,%d2		
	| %d1 = q
	| %d2 = A
	| %d0 = B roll
	movem.l	%d3-%d5,-(%a7)
	| Count leading bits of B
	moveq	#8,%d3
	move.w	%d0,%d5
	cmp.w	#0xff,%d5
	bls.s	0f
		lsr.w	#8,%d5
		moveq	#0,%d3
0:	add.b	CountLeadingZerosTable(%pc,%d5.w),%d3
	swap	%d0
1:		| Count leading bits of A
		moveq	#8,%d4
		move.l	%d2,%d5
		swap	%d5
		cmp.w	#0xff,%d5
		bls.s	0f
			lsr.w	#8,%d5
			moveq	#0,%d4
0:		add.b	CountLeadingZerosTable(%pc,%d5.w),%d4
		sub.w	%d3,%d4	| %d3 > %d4
		beq.s	0f
			addq.w	#1,%d4
			neg.w	%d4	| %d4 = MAX (((32-lead(A))-(32-lead(B))-1), 0)
0:
		moveq	#1,%d5
		lsl.l	%d4,%d5	
		add.l	%d5,%d1	| q+=qq
		move.l	%d0,%d5
		lsl.l	%d4,%d5
		sub.l	%d5,%d2
		cmp.l	%d2,%d0
		bls.s	1b
	movem.l	(%a7)+,%d3-%d5
	rts

CountLeadingZerosTable:
	.if 1
        dc.b    8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
        dc.b    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
        dc.b    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        dc.b    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        dc.b    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        dc.b    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        dc.b    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        dc.b    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.else
	dc.b	7,6,5,5,4,4,4,4,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
	dc.b	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	dc.b	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.endif
