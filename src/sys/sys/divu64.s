| Divide 64-bit unsigned integers
| This routine assumes that the integer part is 1 (high bit is set) in both
| operands. Otherwise Bad Things(tm) might happen. (More technically, the
| condition A < 2*B must hold true.)
| This is primarily suited for the significand portion of extended-precision
| floating-point numbers.

| input:
|  a = %d0:%d1
|  b = %d2:%d3
| output:
|  y = %d4:%d5 = a / b
|  %d0:%d1 contains the remainder
| %d6, %d7 are destroyed
	.long 0xf00dd00f
divu64:
	moveq	#0,%d4
	move.l	%d4,%d5
	move.l	%d4,%d6		| carry for a
	moveq	#63,%d7		| bit counter
	bra	3f		| skip first y <<= 1
0:
	add.l	%d5,%d5		| y <<= 1
	addx.l	%d4,%d4
3:	swap	%d7
	sub.l	%d3,%d1		| a -= b
	subx.l	%d2,%d0
	subx.w	%d7,%d6
	bcc	1f
	| carry is set
	add.l	%d3,%d1		| a += b
	addx.l	%d2,%d0
	addx.w	%d7,%d6
	bra	2f
1:	| carry is clear
	addq	#1,%d5		| y += 1 (no carry)
2:
1:
	add.l	%d1,%d1		| a <<= 1
	addx.l	%d0,%d0
	addx.w	%d7,%d6
	| we could speed this up a little by skipping over 0 bits in A
	| if %d6 is non-zero, jump to 1f
	| if high bit in %d0 is set, jump to 1f
	| y <<= 1
	| dbra %d7,1b
	| rts


1:	swap	%d7
	dbra	%d7,0b
	| It's possible to return a number 0.5 < y < 1
	| If this is the case, we could set a flag and then perform one more
	| iteration of the loop above in order to maximize precision of the
	| quotient (we would get one more bit of precision).
	| The flag (using, eg, %d6) would tell the caller that the quotient was
	| shifted.
	| Alternatively, the caller can pre-shift the numerator left if it is
	| less than the denominator, but the caller also needs a way to inform
	| this routine that it has done this, such as with %d6.
	rts

.global div64
| void div64(int64 a, int64 b, int64 result);

div64:
	movem.l	%d3-%d7,-(%sp)

	move.l	5*4+4(%sp),%a0
	move.l	5*4+4+4(%sp),%a1
	movem.l	(%a0),%d0-%d1
	movem.l	(%a1),%d2-%d3

	bsr	divu64
	move.l	5*4+4+4+4(%sp),%a0
	movem.l	%d4-%d5,(%a0)

	movem.l	(%sp)+,%d3-%d7
	rts
