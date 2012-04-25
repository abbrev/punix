/*
 * fpuemu.s, floating-point unit emulator for 68000
 * Copyright 2011 Christopher Williams
 * 2011-02-24
 * 
 * This attempts to emulate the 68881 FPU by trapping F-line instructions.
 * The instruction is decoded and executed with software floating-point.
 * The 68881 has 8 extended-precision FP data registers (96 bits each). Almost
 * every instruction, with the exception of a few instructions such as fmove,
 * uses a floating-point register as the destination.
 * 
 * So far (as of 2011-02-24 T13:58) only part of the decoding code has been
 * written, and only for the general FP instructions.
 * 
 * A few general notes about this implementation:
 * %a5 is the program counter. use move.w (%a5)+,%dn to get the next word and
 *   advance the PC at the same time
 * %d7 and %d6 are the first and second (if applicable) words of the instruction
 * 
Rounding algorithm:

1. guard + round + sticky = 0: exact result, end
2. set INEX2
3. select rounding mode:

  RN (Round to Nearest):
  a. guard * LSB = 1, round + sticky = 0 OR guard = 1, round + sticky = 1: add 1 to LSB
  b. goto 4

  RM (Round to Minus infinity) and result is negative,
  OR
  RP (Round to Positive infinity) and result is positive:
  a. add 1 to LSB
  b. goto 4

  RZ (Round to Zero):
  a. guard, round, and sticky are chopped
  b. goto 4

3 (alternate). Treat guard, round, and sticky bits as "sub fractions" of the fraction.
  select rounding mode:

  RN (Round to Nearest):
  a. LSB = 1: add 0.011 to fraction (1.001..1.100 => 1.xxx (down), 1.101..1.111 => 0.xxx (up))
     LSB = 0: add 0.100 to fraction (0.001..0.011 => 0.xxx (down), 0.100..0.111 => 1.xxx (up))
  b. goto 4

  RM (Round to Minus infinity) and result is negative,
  OR
  RP (Round to Positive infinity) and result is positive:
  a. add 1 to LSB
  b. goto 4

  RZ (Round to Zero),
  OR
  RM (Round to Minus infinity) and result is positive,
  OR
  RP (Round to Positive infinity) and result is negative:
  a. goto 4

4. overflow = 1:
  a. shift mantissa right 1 bit
  b. add 1 to exponent
5. clear guard, round, and sticky
6. end

Note: I don't see the point of the round bit. It seems to be used always in
conjuction with the sticky bit, so it can probably be eliminated.
Of course, implementing the extra bits is somewhat expensive, so each operation
can keep track of the extra bits its own way so it can perform rounding
reliably and efficiently. For example, FMUL may contain the full 128-bit
product before rounding, so it can extract the guard and sticky bits from the
bottom 64 bits.

 */
	.if 0
MC6888X/MC68040 FPU instructions
taken from M68000PRM.pdf (M68000 Family Programmer's Reference Manual)

instruction coding:

fbcc:
1111iii01spppppp
dddddddddddddddd
dddddddddddddddd

iii = coprocessor id (typically 1)
s = size
pppppp = conditional predicate
dd... = displacement (16 bit and sign-extended if s=0, 32 bit if s=1)
special case:
 fnop = pppppp=0, s=0


fdbcc:
1111iii001001ccc
0000000000pppppp
dddddddddddddddd

ccc = count register
pppppp = conditional predicate


fscc:
1111iii001eeeeee
0000000000pppppp

pppppp = conditional predicate


ftrapcc:
1111iii001111mmm
0000000000pppppp
dddddddddddddddd
dddddddddddddddd

mmm = mode
 010 word operand
 011 long-word operand
 100 no operand

pppppp = conditional predicate
ddd... = 16- or 32-bit operand (if needed)



************************************************************************

coprocessor instruction formats

first word:
1111iiinnnxxxxxx

iii is the coprocessor ID (FPU is usually 001)
nnn determines how the rest of the bits are interpreted
 000 general instruction
 001 Scc/DBcc
 01s Bcc (s=size of displacement)
 100 save
 101 restore
xxxxxx is almost always a 6-bit field
	.endif

.equiv fpregsize,12
.data
|fpregs: .ds.b	8*fpregsize	| 8 registers, 96 bits each
|srcfpreg: .ds.b	12
.equ	fpregs,0x5c00+4+8+60	| see globals.h
.equ	srcfpreg,fpregs+8*fpregsize

| fp control and status registers

|fpcr:	.ds.l	1	| Floating-point control register
.equ	fpcr,srcfpreg+fpregsize
.equ	fpcrenable,fpcr+2
.equ	fpcrmode,fpcr+3

|fpsr:			| Floating-point status register
|fpsrcc:	.ds.b	1	| FPSR Condition Code Byte
|fpsrquo:	.ds.b	1	| FPSR Quotient Code Byte
|fpsrexc:	.ds.b	1	| FPSR Exception Byte
|fpsraexc:	.ds.b	1	| FPSR Accrued Exception Byte
.equ	fpsr,fpcr+1*4
.equ	fpsrcc,fpcr+0
.equ	fpsrquo,fpcr+1
.equ	fpsrexc,fpcr+2
.equ	fpsraexc,fpcr+3

|fpia:	.ds.l	1	| Floating-point instruction address register
.equ	fpia,fpsr+4

.equiv	CC_NAN_BIT, 0
.equiv	CC_I_BIT,   1
.equiv	CC_Z_BIT,   2
.equiv	CC_N_BIT,   3
.equiv	CC_NAN, (1<<CC_NAN_BIT)
.equiv	CC_I,   (1<<CC_I_BIT)
.equiv	CC_Z,   (1<<CC_Z_BIT)
.equiv	CC_N,   (1<<CC_N_BIT)

.equiv	FPCRMODE_RND_MASK,  0x30
.equiv	FPCRMODE_PREC_MASK, 0xc0

| these bits apply to the FPCR ENABLE byte and FPSR EXC byte
.equiv	EXC_INEX1_BIT, 0
.equiv	EXC_INEX2_BIT, 1
.equiv	EXC_DZ_BIT,    2
.equiv	EXC_UNFL_BIT,  3
.equiv	EXC_OVFL_BIT,  4
.equiv	EXC_OPERR_BIT, 5
.equiv	EXC_SNAN_BIT,  6
.equiv	EXC_BSUN_BIT,  7

.equiv	AEXC_INEX_BIT, 3	| INEX1 + INEX2 + OVFL
.equiv	AEXC_DZ_BIT,   4	| DZ
.equiv	AEXC_UNFL_BIT, 5	| UNFL L INEX2
.equiv	AEXC_OVFL_BIT, 6	| OVFL
.equiv	AEXC_IOP_BIT,  7	| SNAN + OPERR

| format of extended-precision float register:
| seee eeee eeee eeee 0000 0000 0000 0000
| ffff ffff ffff ffff ffff ffff ffff ffff
| ffff ffff ffff ffff ffff ffff ffff ffff
| 
| s=sign
| e=exponent
| f=fraction (with explicit integer bit)
| 
| N.B. Internally ext float values are stored with an unbiased 16-bit exponent,
| a sign in bit 16, and an inf/nan flag in bit 17, but they are stored in
| memory as above.
| This internal format simplifies detecting overflow/underflow
| N.B. The NAN, INF, Z, and SIGN bits correspond exactly with the condition
| code bits so that testing each condition is trivial.
.equiv EXT_NAN_BIT,  16 | set=nan
.equiv EXT_INF_BIT,  17 | set=inf
.equiv EXT_Z_BIT,    18 | set=zero
.equiv EXT_SIGN_BIT, 19 | set=negative
.equiv EXT_NAN,  (1<<EXT_NAN_BIT)
.equiv EXT_INF,  (1<<EXT_INF_BIT)
.equiv EXT_Z,    (1<<EXT_Z_BIT)
.equiv EXT_SIGN, (1<<EXT_SIGN_BIT)

.equiv EXT_EXP_BIAS, 16383
.equiv EXT_EXP_MAX,  32766

.equiv SNG_EXP_BIAS, 127
.equiv SNG_EXP_MAX,  254

.equiv DBL_EXP_BIAS, 1023
.equiv DBL_EXP_MAX,  2046

.text


	| offsets from %fp
	.equ SAVED_D0,4
	.equ SAVED_D1,8
	.equ SAVED_D2,12 
	.equ SAVED_D3,16 
	.equ SAVED_D4,20 
	.equ SAVED_D5,24 
	.equ SAVED_D6,28 
	.equ SAVED_D7,32 
	.equ SAVED_A0,36 
	.equ SAVED_A1,40 
	.equ SAVED_A2,44 
	.equ SAVED_A3,48 
	.equ SAVED_A4,52 
	.equ SAVED_A5,56 
	.equ SAVED_A6,60 
	.equ SAVED_A7,64 
	.equ SAVED_SP,64 
	.equ SAVED_SR,68
	.equ SAVED_PC,70 

| line F trap entry point
	.global fpuemu
fpuemu:
	| steps:
	| 0. save all registers (d0-d7, a0-a6, usp)
	movem.l	%d0-%d7/%a0-%a7,-(%sp)
	link	%fp,#0
	move.l	%usp,%a0
	move.l	%a0,(SAVED_SP,%fp)
	move.l	(SAVED_PC,%fp),%a5
	
	| 1. verify that coproc ID (bits 9-11) is 001 (is this necessary?)
	move	(%a5)+,%d7	| get first instruction word
	move	%d7,%d0
	and	#7*0x200,%d0
	cmp	#1*0x200,%d0
	bne	8f
1:
	
	| 2. decode instruction type (bits 6-8 of first instruction word)
	move	%d7,%d0
	lsr.w	#6-2,%d0
	and	#4*7,%d0
	
	| 3. jump to handler for that instruction type
	move.l	(7f,%pc,%d0.w),%a0
	jsr	(%a0)
	
	| 4. in the handlers, decode the rest of the fields and execute the
	|    instruction

9:
	| add exceptions to accrued exceptions
	| FIXME: accrued exception byte has a different layout than the exception byte
	move.b	fpsrexc,%d0
	or.b	%d0,fpsraexc
	
	| 5. restore all registers
	move.l	%a5,(SAVED_PC,%fp)
	move.l	(SAVED_SP,%sp),%a0
	move.l	%a0,%usp
	unlk	%fp
	movem.l	(%sp)+,%d0-%d7/%a0-%a6
	lea.l	(4,%sp),%sp
	
	| 6. return from trap
	| TODO: check and post pending signals etc
	rte

7:
	.long	instr_gen
	.long	instr_scc
	.long	instr_bcc_w
	.long	instr_bcc_l
	.long	instr_save
	.long	instr_restore
	.long	instr_invalid
	.long	instr_invalid

8:
	bsr	instr_invalid
	bra	9b

.equiv FORMAT_INT_LONG, 0
.equiv FORMAT_REAL_SNG, 1
.equiv FORMAT_REAL_EXT, 2
.equiv FORMAT_REAL_PKD, 3
.equiv FORMAT_INT_WORD, 4
.equiv FORMAT_REAL_DBL, 5
.equiv FORMAT_INT_BYTE, 6
.equiv FORMAT_INVALID,  7 | this is used for fmovecr

| input:
|  %d7=first instruction word
|  %d0=data size (determines pre-decrement and
|                 post-increment size)
| output:
|  %a0=pointer to (source) operand
|  %d0=type of EA (bit field):
|  %d0 is 0 if invalid
|  Z flag is set if invalid
.equiv EA_WRITABLE_BIT, 0
.equiv EA_MEMORY_BIT,   1
.equiv EA_PREDEC_BIT,   2
.equiv EA_POSTINC_BIT,  3
.equiv EA_DATAREG_BIT,  4
.equiv EA_ADDRREG_BIT,  5

.equiv EA_WRITABLE, (1<<EA_WRITABLE_BIT)
.equiv EA_MEMORY,   (1<<EA_MEMORY_BIT)
.equiv EA_PREDEC,   (1<<EA_PREDEC_BIT)	| this can be repeated safely
.equiv EA_POSTINC,  (1<<EA_POSTINC_BIT)	| this can be repeated safely
.equiv EA_DATAREG,  (1<<EA_DATAREG_BIT)
.equiv EA_ADDRREG,  (1<<EA_ADDRREG_BIT)
.equiv EA_PREPOST, EA_PREDEC+EA_POSTINC
get_ea:
	| TODO: verify word and long memory accesses are word-aligned
	move	%d7,%d2
	move	%d7,%d1
	lsr	#3,%d2
	and	#7,%d2		| EA mode field
	and	#7,%d1		| EA register field




	| XXX: please test all of these addressing modes
1:	cmp	#7,%d2
	bne	3f
	| (xxx).W / (xxx).L / #<data> / (d16,PC) / (d8,PC,Xn)
	.if 0
mode	register	addressing mode
111	000		(xxx).W
111	001		(xxx).L
111	010		(d16,PC)
111	011		(d8,PC,Xn)
111	011		(bd,PC,Xn)		not on 68000
111	011		([bd,PC,Xn],od)		not on 68000
111	011		([bd,PC],Xn,od)		not on 68000
111	100		#<data>
	.endif
	dbra	%d1,1f	| %d1 == 0?
	| (xxx).W
	move.w	(%a5)+,%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d1,1f	| %d1 == 1?
	| (xxx).L
	move.l	(%a5)+,%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d1,1f	| %d1 == 2?
	| (d16,PC) -- tested, works
	move	(%a5)+,%d1
	cmp	#1,%d0
	beq	0f
	btst	#0,%d1
	beq	0f
	| unaligned access--invalid
	moveq.l	#0,%d0
	rts
0:	lea.l	(-2,%a5,%d1.w),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d1,1f	| %d1 == 3?
	| (d8,PC,Xn)
	move.l	(SAVED_PC,%fp),%a3
	move	(%a5)+,%d2
	move	%d2,%d1
	ext.w	%d2		| d8
	lsr	#8,%d1		| register number
	move.l	(SAVED_D0,%fp,%d1.w),%a0
	| XXX: is Xn word or long? 
	| TODO: if d8+Xn is odd and size is not 1, return invalid
	add.w	%a0,%d2
	lea.l	(%a3,%a0.w),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d1,1f	| %d1 == 4?
	| #<data> -- tested, works
	cmp	#4,%d0
	bne	2f
	| long
	lea.l	(%a5),%a0
	addq	#4,%a5
	move	#EA_MEMORY,%d0
	rts
2:	cmp	#2,%d0
	bne	2f
	| word
	lea.l	(%a5),%a0
	addq	#2,%a5
	move	#EA_MEMORY,%d0
	rts
2:	cmp	#1,%d0
	bne	2f
	| byte
	lea.l	(1,%a5),%a0
	addq	#2,%a5
	move	#EA_MEMORY,%d0
	rts
2:	| invalid! (not sure how to get this condition)
	moveq	#0,%d0
	rts

	.if 0
Effective address field:
mode	register	addressing mode
000	Dn		Dn
001	An		An
010	An		(An)
011	An		(An)+
100	An		-(An)
101	An		(d16,An)
110	An		(d8,An,Xn)
110	An		(bd,An,Xn)		not on 68000
110	An		([bd,An,Xn],od)		not on 68000
110	An		([bd,An],Xn,od)		not on 68000
	.endif

3:	lsl	#2,%d1	| %d1 = 4 * register number
	dbra	%d2,1f	| %d2 == 0?
	| Dn -- TEST AGAIN
	cmp	#4,%d0
	ble	0f
	moveq.l	#0,%d0	| invalid size for Dn
	rts
0:	| add (4-size) to offset into register (byte: 3, word: 2, long: 0)
	subq	#4,%d0
	sub	%d0,%d1
	
	lea.l	(SAVED_D0,%fp,%d1.w),%a0
	move	#EA_DATAREG+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 1?
	| An
	cmp	#4,%d0
	beq	0f
	moveq.l	#0,%d0	| invalid size for An
	rts
0:	lea.l	(SAVED_A0,%fp,%d1.w),%a0
	move	#EA_ADDRREG+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 2?
	| (An) -- tested, works
	move.l	(SAVED_A0,%fp,%d1.w),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	| TODO: check alignment of %a0
	rts

1:	dbra	%d2,1f	| %d2 == 3?
	| (An)+ -- tested, works
	lea.l	(SAVED_A0,%fp,%d1.w),%a3

	cmp	#4*7,%d1	| is this A7?
	bne	0f
	cmp	#1,%d0		| is the size 1?
	bne	0f
	moveq	#2,%d0		| (A7)+ with size=1
	| N.B. a byte is stored in the upper half of a word on the stack,
	| such that %sp points to the byte after "move.b #0,-(%sp)" executes
0:	move.l	(%a3),%a0	| get (An)
	add.w	%d0,(%a3)	| postincrement An
	move	#EA_POSTINC+EA_MEMORY+EA_WRITABLE,%d0
	| TODO: check alignment of %a0
	rts

1:	dbra	%d2,1f	| %d2 == 4?
	| -(An) -- tested, works
	lea.l	(SAVED_A0,%fp,%d1.w),%a3

	cmp	#4*7,%d1
	bne	0f
	cmp	#1,%d0
	bne	0f
	moveq	#2,%d0		| -(A7) with size=1
0:	sub.w	%d0,(%a3)	| predecrement An
	move.l	(%a3),%a0	| get (An)
	move	#EA_PREDEC+EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 5?
	| (d16,An) -- tested, works
	move.l	(SAVED_A0,%fp,%d1.w),%a0
	move	(%a5)+,%d0
	lea.l	(%a0,%d0.w),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	| TODO: check alignment of %a0
	rts

1:	| %d2 == 6
	| (d8,An,Xn)
	move.l	(SAVED_A0,%fp,%d1.w),%a0
	move	(%a5)+,%d0
	move	%d0,%d1		| index register field
	move	%d0,%d2		| d8 field
	and	#0xff,%d2	| d8
	lsr	#6,%d1
	and	#124,%d1	| register number 4*[0..15]
	move.l	(SAVED_D0,%fp,%d1.w),%d1
	btst	#11,%d0		| size of Xn register
	bne	3f
	ext.l	%d2		| treat Xn register as word
3:
	add	%d2,%a3		| Xn + d8
	move.l	(%a0,%d1.l),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	| TODO: check alignment of %a0
	rts
	

| input:
|  %d7 = first instruction word
|  %d6 = second instruction word
| output:
|  %d0-%d2 = source value
|  Z flag is set if source is invalid
|  %d3 is clobbered
get_src:
	| get src format
	move	%d6,%d0
	rol	#6,%d0
	and	#7,%d0

	move	%d0,%d3		| save format
	move.b	(format_sizes,%pc,%d0.w),%d0	| get the format size
	ext.w	%d0
	btst	#14,%d6		| R/M bit
	beq	0f

	| get ea
	jbsr	get_ea		| get source from memory
	beq	9f		| Z flag is set on error
	move	%d3,%d0		| restore format
	jbsr	convert_to_ext
	moveq	#1,%d3
	rts


0:	| get the source fp register
	mulu	#12,%d3

	lea.l	fpregs,%a0
	lea.l	(%a0,%d3.w),%a0
	movem.l	(%a0),%d0-%d2
	moveq	#1,%d3		| clear Z flag (source is valid)
9:	rts

| input:
|  %d6 = second instruction word
| output:
|  %a4 = pointer to destination fp data register
get_dst:
	| get the destination fp register
	move	%d6,%d0

	lsr	#7-3,%d0
	and	#7*8,%d0
	move	%d0,%d1
	lsr	#1,%d0
	add	%d1,%d0		| multiplied by 12

	lea.l	fpregs,%a4
	lea.l	(%a4,%d0.w),%a4
	rts

| input:
|  %d7 = first instruction word
|  %d6 = second instruction word
| output:
|  %d0 = format
get_format:
	move	%d6,%d0
	lsr	#8,%d0
	lsr	#2,%d0
	and	#7,%d0
	rts

| input:
|  %d0 = format (L/S/X/P/W/D/B)
| output:
|  %d0 = size of format
get_format_size:
	move.b	(format_sizes,%pc,%d0.w),%d0
	ext.w	%d0
	rts

format_sizes:
	.byte	4	| long-word integer
	.byte	4	| single-precision real
	.byte	12	| extended-precision real
	.byte	12	| packed-decimal real
	.byte	2	| word integer
	.byte	8	| double-precision real
	.byte	1	| byte integer
	.byte	0	| invalid format

instr_gen:
	move	(%a5)+,%d6	| get next instruction word
	
	move	%d6,%d0
	and	#0xfc00,%d0	| get the top 6 bits of second instruction word
	
	cmp	#0x5c00,%d0	| 010111
	beq	fmovecr		| fmovecr
	
	and	#0xa000,%d0	| 101000 <- either bit set
	bne	fmove_misc	| misc fmove
	
	| regular arithmetic instruction
	| steps to complete an arithmetic instruction:
	| 1. get src (converted to extended-precision)
	| 2. get pointer to dst
	| 3. call the specific arithmetic operation handler (eg, instr_gen_fadd etc)
	| 4. the handler loads operands as needed and calls the proper operator (eg, fadd)
	| 5. the operator routine performs the operation and sets exceptions
	| 5a. operator routine also rounds and then normalizes the result
	| 6. the handler then (typically) saves the result in the destination, tests the
	|    result (ftst) to set condition codes, and returns back here
	| step 5 is separate from the instruction handler so as to make aritmetic operations
	| reusable in higher-level operations (eg, fetox, fsin)
	bsr	get_dst
	bsr	get_src
	beq	instr_invalid	| valid?
	move	%d6,%d3
	and	#127,%d3
	lsl	#2,%d3
	move.l	%a4,%a1		| destination fp register
	
	| %d0-%d2 = source extended-precision value
	| %a1 = destination register
	move.l	(7f,%pc,%d3.w),%a3
	| movem.l	(%a1),%d3-%d5	| pre-load destination value
	jsr	(%a3)
	| TODO: set exception byte and condition codes in %d6 in each
	| arithmetic routine, then move that to fpsr here and take an exception
	| if any exceptions are set and enabled
	rts
/*
 * All arithmetic instruction handlers take the source value in %d0-%d2
 * (signficand in %d1-%d2, unbiased exponent in %d0.w, and sign in bit 16 of
 * %d0.l).
 * TODO: change code to use an unbiased exponent and bit 16 of %d0 for the sign.
 * XXX: an unbiased exponent makes inf and nan more difficult to represent.
 * We should also set a bit or two in the upper word of %d0 to indicate that
 * it's inf or nan. One bit (eg, bit 17) should suffice because inf and nan are
 * differentiated by the significand (inf: fraction is zero, nan: fraction is
 * non-zero).
 * 
 * The low-level arithmetic routines generally accept values in %d0-%d2 (source)
 * and %d3-%d5 (destination) which simplifies call sequences.
 * 
 * N.B. All arithmetic routines, with the exception of fcmp for some reason,
 * set the condition codes the same way. fcmp always clears the I condition
 * code since it's not used in any conditional predicate. If we are willing to
 * sacrifice a slight bit of accuracy here we can set condition codes in a
 * common place (eg, at the end of instr_gen).
 * 
 * TODO: see whether keeping the exponent and sign in the same word in %d0 or
 * separate (eg, LSW=exponent, MSW=sign) is more efficient and easier to work
 * with overall. %d0 has to store the exponent and sign in the MSW by the time
 * it's written to the destination (via an instruction like
 * movem.l %d0-%d2,(%a3)), but they can be separate up to that point. Perhaps
 * each arithmetic instruction handler can do its own separation (or call a
 * common routine) if it desires.
 */


7:	| instr_gen table
	.long instr_gen_fmove	| 0
	.long instr_gen_fint
	.long instr_gen_fsinh
	.long instr_gen_fintrz
	.long instr_gen_fsqrt
	.long instr_gen_invalid
	.long instr_gen_flognp1
	.long instr_gen_invalid
	.long instr_gen_fetoxm1	| 8
	.long instr_gen_ftanh
	.long instr_gen_fatan
	.long instr_gen_invalid
	.long instr_gen_fasin
	.long instr_gen_fatanh
	.long instr_gen_fsin
	.long instr_gen_ftan
	.long instr_gen_fetox	| 16
	.long instr_gen_ftwotox
	.long instr_gen_ftentox
	.long instr_gen_invalid
	.long instr_gen_flogn
	.long instr_gen_flog10
	.long instr_gen_flog2
	.long instr_gen_invalid
	.long instr_gen_fabs	| 24
	.long instr_gen_fcosh
	.long instr_gen_fneg
	.long instr_gen_invalid
	.long instr_gen_facos
	.long instr_gen_fcos
	.long instr_gen_fgetexp
	.long instr_gen_fgetman
	.long instr_gen_fdiv	| 32
	.long instr_gen_fmod
	.long instr_gen_fadd
	.long instr_gen_fmul
	.long instr_gen_fsgldiv
	.long instr_gen_frem
	.long instr_gen_fscale
	.long instr_gen_fsglmul
	.long instr_gen_fsub	| 40
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_fsincos	| 48
	.long instr_gen_fsincos
	.long instr_gen_fsincos
	.long instr_gen_fsincos
	.long instr_gen_fsincos
	.long instr_gen_fsincos
	.long instr_gen_fsincos
	.long instr_gen_fsincos
	.long instr_gen_fcmp	| 56
	.long instr_gen_invalid
	.long instr_gen_ftst
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 64
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 72
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 80
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 88
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 96
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 104
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 112
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid	| 120
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid
	.long instr_gen_invalid



fmove_misc:	| misc fmove
	| TODO
	| these fmove instructions may treat the EA as the destination
| 001 invalid?
| 011 fmove reg-mem
| 10D fmovem system control registers
| 11D fmovem data registers
	
	move	%d6,%d0
	and	#0xe000,%d0	| get the top 3 bits of second instruction word
	
	cmp	#0x6000,%d0	| 011 fmove reg-mem
	beq	0f
	and	#0xc000,%d0	| top 2 bits
	cmp	#0x8000,%d0	| 10 fmovem system control registers
	beq	1f
	cmp	#0xc000,%d0	| 11 fmovem data registers
	jbne	instr_invalid
	
	| fmovem data registers
| fmovem (data registers):
| 1111iii000eeeeee
| 11Dmm000rrrrrrrr
|
| D = direction of data transfer
|  0=from memory to fpu
|  1=from fpu to memory
| mm = mode
|  00 static register list, predecrement addressing mode (reg-mem only)
|  01 dynamic register list, predecrement addressing mode (reg-mem only)
|  10 static register list, postincrement or control addressing mode
|  11 dynamic register list, postincrement or control addressing mode
| rrrrrrrr = register list
| 
	| TODO
	rts
1:	| fmovem system control registers
| fmovem (system control registers)
| 1111iii000eeeeee
| 10Drrr0000000000
| 
| rrr = register list
|  100 floating-point control register (FPCR)
|  010 floating-point status register (FPSR)
|  001 floating-point instruction address register (FPIAR)
| 
	| TODO
	rts
0:	| fmove reg-mem
| fmove (register to memory):
| 1111iii000eeeeee
| 011dddssskkkkkkk
| kkkkkkk = k-factor (required only for packed decimal destination)
| 
	bsr	get_dst		| this actually gets the source fp register
	bsr	get_format	| get format of destination
	move	%d0,-(%sp)
	.if 1
	jbsr	get_format_size
	.else
	lea.l	(format_sizes,%pc),%a0
	move.b	(%a0,%d0.w),%d0
	ext.w	%d0
	.endif
	move	%d0,-(%sp)
	bsr	get_ea		| get destination ea
	beq	instr_invalid	| valid?
	move	(%sp)+,%d1	| format size
	move	(%sp)+,%d2	| format
	| %a4 = source reg
	| %a0 = dest ea
	| %d0 = EA type
	| %d1 = format size
	| %d2 = format
	
	| verify that destination type is compatible with format
	bsr	ea_is_valid_dst
	bne	instr_invalid
	
	move	%d2,%d6	| format
	movem.l	(%a4),%d0-%d2	| load dst register
	move.l	%a0,%a1	| destination
	|jbsr	ftst
	jbra	convert_from_ext

| input:
|  %d0 = EA type
|  %d1 = format size
| output:
|  zero is set if valid
| %d3 is clobbered
ea_is_valid_dst:
	btst	#EA_WRITABLE_BIT,%d0
	beq	1f
	| fall through (a valid dst is also a valid src--most of the time)

| input:
|  %d0 = EA type
|  %d1 = format size
| output:
|  zero is set if valid
| %d3 might be clobbered
ea_is_valid_src:
	btst	#EA_ADDRREG_BIT,%d0		| address registers are not allowed as src or dst
	bne	2f
	btst	#EA_MEMORY_BIT,%d0
	bne	0f
	| it's a data register; verify that format size is <= 4
	cmp	#4,%d1
	ble	0f
1:	move	#1,%d3		| invalid
	rts
0:	move	#0,%d3		| valid
2:	rts

| fmove reg-reg or mem-reg
instr_gen_fmove:
| fmove (memory/register to register):
| 1111iii000eeeeee
| 0R0sssdddooooooo
|
| sss = source register or format
| ddd = destination register
| ooooooo = opmode
|  0000000 fmove  rounding precision specified by the floating-point control register
|  1000000 fsmove single-precision rounding specified *
|  1000100 fdmove double-precision rounding specified *
|  * supported by MC68040 only
| 
| R = R/M field
|  0 = source is register
|  1 = source is <ea>
| 
	| %d0-%d2 = src
	| %a1 = dst
	| fall through

| input:
|  %d0-%d2 = value to save
|  %a1 = destination
| output:
|  condition codes are set
| This routine is called by nearly all arithmetic instruction handlers to set
| condition codes and then save the result to the destination register.
| TODO: round using current rounding mode and precision
ftst_and_save_dst:
	jbsr	ftst
save_dst:
	movem.l	%d0-%d2,(%a1)
	rts


| check and try to handle nan
| input:
|  %d0-%d2 = src
|  %d3-%d5 = dst
| output:
|  %d0-%d2 = result
|  zero is clear if nan handled
|  %d6 is clobbered
| 
| src  dst  result
| ?    nan  nan (from dst)
| nan  n    nan (from src)
| 
| signaling nan is converted to quiet nan
check_nan:
	btst.l	#EXT_NAN_BIT,%d3
	beq	3f	| dst is not nan
	| dst is nan; copy it to result
	move.l	%d3,%d0
	move.l	%d4,%d1
	move.l	%d5,%d2
1:	| result is nan
	bset.l	#30,%d1	| set quiet bit (clears z flag)
	rts
3:	| dst is not nan; check src
	btst.l	#EXT_NAN_BIT,%d0
	bne	1b
	rts		| z flag is set

| y = integer part of x
instr_gen_fint:
	| TODO
	rts

| y = sinh(x)
instr_gen_fsinh:
	| TODO
	rts

| y = integer part of x, round to zero
instr_gen_fintrz:
	| TODO
	rts

| y = sqrt(x)
instr_gen_fsqrt:
	| TODO
	rts

| y = ln(x+1)
instr_gen_flognp1:
	| TODO
	rts

| y = e^(x-1)
instr_gen_fetoxm1:
	| TODO
	rts

| y = tanh(x)
instr_gen_ftanh:
	| TODO
	rts

| y = atan(x)
instr_gen_fatan:
	| TODO
	rts

| y = asin(x)
instr_gen_fasin:
	| TODO
	rts

| y = atanh(x)
instr_gen_fatanh:
	| TODO
	rts

| y = sin(x)
instr_gen_fsin:
	| TODO
	rts

| y = tan(x)
instr_gen_ftan:
	| TODO
	rts

| y = e^x
instr_gen_fetox:
	| TODO
	rts

| y = 2^x
instr_gen_ftwotox:
	| TODO
	rts

| y = 10^x
instr_gen_ftentox:
	| TODO
	rts

| y = ln(x)
instr_gen_flogn:
	| TODO
	rts

| y = log10(x)
instr_gen_flog10:
	| TODO
	rts

| y = log2(x)
instr_gen_flog2:
	| TODO
	rts

| y = abs(x)
instr_gen_fabs:
	clr.b	fpsrexc
	bclr	#EXT_SIGN_BIT,%d0
	jbra	ftst_and_save_dst

| input:
|  %d0-%d2 = ext float
| output:
|  %d0-%d2 = ext float with its sign cleared
fabs:
	bclr	#EXT_SIGN_BIT,%d0
	rts

| y = cosh(x)
instr_gen_fcosh:
	| TODO
	rts

| y = acos(x)
instr_gen_facos:
	| TODO
	rts

| y = cos(x)
instr_gen_fcos:
	| TODO
	rts

| y = getexp(x)
instr_gen_fgetexp:
	| TODO
	rts

| y = getman(x)
instr_gen_fgetman:
	| TODO
	rts

| y = y / x
instr_gen_fdiv:
	movem.l	(%a1),%d3-%d5	| load dst register
	jbsr	fdiv
	jbra	ftst_and_save_dst

| y = y / x
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d3-%d5 = y / x
fdiv:
	| TODO
	rts

	.if 0
	| single precision divide (core code from libgcc)
	moveq	FLT_MANT_DIG+1,%d3
1:	cmp.l	%d1,%d0
	blo	2f
	bset	%d3,%d6
	sub.l	%d1,%d0
	beq	3f
2:	add.l	%d0,%d0
	dbra	%d3,1b

	| find the sticky bit
	moveq	#FLT_MANT_DIG,%d3
1:	cmp.l	%d0,%d1
	ble	2f
	add.l	%d0,%d0
	dbra	%d3,1b
	
	moveq.l	#0,%d1
	bra	3f
2:	moveq.l	#0,%d1
	add	#31-FLT_MANT_DIG,%d3
	bset	%d3,%d1

	|
3:
	move.l	%d6,%d0		| put the ratio in %d0-%d1
	move.l	%a0,%d7		| get sign back

	btst	FLT_MANT_DIG+1,%d0
	beq	1f
	lsr.l	#1,%d0
	add	#1,%d2
1:
	| round etc



	| extended precision version:
	| numerator = %d4-%d5
	| denominator = %d1-%d2
	| result = %d6:%d7
	| loop counter = %d3 (or a mask in alternate version)
	| unused: %d0/%a0-%a7
	moveq	#0,%d6
	move.l	%d6,%d7

	moveq	31,%d3
1:	cmp.l	%d1,%d4
	blo	2f	| numerator < denominator?
	bhi	5f	| numerator > denominator?
	cmp.l	%d2,%d5
	blo	2f
5:	bset	%d3,%d6	| numerator >= denominator
	cmp	%d0,%d0	| set z flag
	sub.l	%d2,%d5
	subx.l	%d1,%d4
	beq	3f	| it's zero
2:	add.l	%d5,%d5	| shift numerator left 1 bit
	addx.l	%d4,%d4
	bcs	6f	| numerator > denominator (guaranteed)
4:	dbra	%d3,1b
	bra	8f	| exit

1:	| this loop is used when carry is set after %d4 is shifted left
	| numerator must be greater than denominator
	bset	%d3,%d6	| numerator > denominator
	cmp	%d0,%d0
	sub.l	%d2,%d5
	subx.l	%d1,%d4
	add.l	%d5,%d5	| shift numerator left 1 bit
	addx.l	%d4,%d4
	bcc	4b
6:	dbra	%d3,1b
8:	| exit

	| repeat the above for the low longword (%d7)
	| then find the sticky bit
	.endif

| y = y modulo x (round to zero)
instr_gen_fmod:
	| TODO
	rts

	.global sr64
| s{l,r}64_reg shift the 64-bit integer in %d1:%d2 (%d1 is upper 32 bits) left
| or right by the unsigned shift amount in %d0.
| 
| If shift amount is 32 or greater, but less than 64, value is first shifted 32
| bits by moving one register to the other, then the remaining register is
| shifted by the shift amount minus 32.
| 
| Otherwise (shift amount is 64 or greater), the value is cleared to 0.
|
| Here is a quick demonstration of how shifting right works:
| (threshold <= shift amount < 32)
| %d1  %d2  %d0  %d3
|  AB   CD            input
|  BA   0C            rotate %d0 and shift %d1 right by %d2
|  BA   0C   10   01  compute masks
|  BA   0C   B0   01  upper bits from %d0
|  BA   BC   B0   01  put upper bits from %d0 into %d1
|  0A   BC   B0   01  clear upper bits in %d0
|
| Shifting left is almost exactly the same.

| unsigned long long sr64(unsigned long long, unsigned);
sr64:
	movem.l	%d2-%d7,-(%sp)
	move.l	(6*4+4,%sp),%d1
	move.l	(6*4+8,%sp),%d2
	move.w	(6*4+12,%sp),%d0
	bsr	sr64_reg
	move.l	%d1,%d0
	move.l	%d2,%d1
	movem.l	(%sp)+,%d2-%d7
	rts
	
| shift right a 64-bit number
| input:
|  %d1:%d2 = 64-bit number (%d1 is upper 32 bits)
|  %d0.w = shift amount (unsigned)
| output:
|  %d1:%d2, shifted
| TODO output:
|  %d7 = rounding bits (bit #31 is last bit shifted out, bit #30 is second
|    last, and bits #29 to #0 are "sticky" bits
| TODO: a nice addition to this would be to save the "rounding" bits, ie, save
| at least one (two?) bit below the LSb of the result, and save the
| inclusive OR of all other bits below that. The first bit is the equivalent of
| the guard and round bits on the 68881, and the last part is the sticky bit.
| 
| basic algorithm for shifting right by less than the word width:
| ab cd
| ba dc rotated right
|       dc move low word to rounding bits
|    dc
|    01 and
|    0c
|       dc
|       0c eor
|       d0 final rounding bits
|    0c
|    ba eor
|    b? perturbed low word
| ba
| 01 and
| 0a
|    b?
|    0a eor
|    bc final low word
| 0a final high word
| TODO: clean this up and make sure that %d7 contains rounding bits as follows:
| grssssssssssssssssssssssssss
| where g=guard, r=round, s=sticky
| any 's' bit can be set to indicate sticky
sr64_reg:
	move.l	#0,%d7
	tst	%d0
	beq	9f
	sub	#32,%d0
	bhi	5f
	| 0 <= amount <= 32
0:	beq	1f
	| 0 <= amount < 32
	ror.l	%d0,%d1
	ror.l	%d0,%d2
	
	| TODO: test this!
	| compute mask
	moveq	#0,%d6
	neg	%d0
	bset	%d0,%d6
	subq.l	#1,%d6		| 00..11 (lower bits)

	move.l	%d2,%d7		| rounding bits
	and.l	%d6,%d2
	eor.l	%d2,%d7		| final rounding bits
	eor.l	%d1,%d2		| perturbed low word
	and.l	%d6,%d1		| final high word
	eor.l	%d1,%d2		| final low word
	
9:	rts
1:	| amount = 32
	move.l	%d2,%d7
	move.l	%d1,%d2
	move.l	#0,%d1
	rts

5:
	cmp	#32,%d0
	bhi	6f
	| 32 < amount <= 64
	bsr	0b	| shift right using above code
	| collapse sticky bits into one bit
	tst.l	%d7
	beq	2f
	moveq.l	#1,%d7
2:	or.l	%d2,%d7	| rounding bits
	move.l	%d1,%d2
	move.l	#0,%d1
	rts

6:
	| 64 < amount
	cmp	#65-32,%d0
	bhi	1f
	| amount = 65
	move.l	#0,%d7
	lsr.l	#1,%d1
	roxr.l	#1,%d2
	addx.l	%d7,%d7	| preserve carried bit
	or.l	%d2,%d7
3:	beq	2f
	moveq.l	#1,%d7
2:	or.l	%d1,%d7	| rounding bits
	bra	9f
1:	| amount > 65
	or.l	%d1,%d2
	move.l	%d2,%d7
	beq	2f
	move.l	#1,%d7
2:
9:	| all bits in original value are shifted out
	moveq.l	#0,%d1
	move.l	%d1,%d2
	rts


	.global sl64
| unsigned long long sl64(unsigned long long, unsigned);
sl64:
	move.l	(4,%sp),%d1
	move.l	(8,%sp),%d2
	move.w	(12,%sp),%d0
	
| shift left a 64-bit number
| input:
|  %d1:%d2 = 64-bit number (%d1 is upper 32 bits)
|  %d0.w = shift amount (unsigned)
| output:
|  %d1:%d2, shifted
| %d6 is clobbered
sl64_reg:
	cmp	#32,%d0
	bhs	5f
	| 0 <= amount < 32
	rol.l	%d0,%d2
	lsl.l	%d0,%d1		| xx..00 (lower bits cleared)
	
	| awesome optimized code from Samuel Stearley (thanks man!)
	| compute masks
	moveq   #-1,%d6
	bclr.l  %d0,%d6
	addq.l  #1,%d6     | 11..00 (upper bits)

	eor.l  %d2,%d1     | fill in the lower bits and perturb the upper bits
	and.l  %d6,%d2     | clear the lower bits
	eor.l  %d2,%d1     | un-perturb the upper bits.
	
	rts

5:
	cmp	#64,%d0
	bhs	6f
	| 32 <= amount < 64
	sub	#32,%d0
	move.l	%d2,%d1
	lsl.l	%d0,%d1
	moveq	#0,%d2
	rts

6:
	| 64 <= amount
	moveq.l	#0,%d1
	move.l	%d1,%d2
	rts

| TODO: write a routine to get types of src and dst (stored in %d0-%d2 and
| %d3-%d5) and return types in %d6 or %d7 (leave %d0-%d5 untouched) so it can be
| used as an index into jump tables in any arith operation that wants to do so.
| types: zero, in-range, inf, nan
| each type fits in 2 bits
| this will give a 4x4 (16 entry) jump table

| TODO: write a routine or routines to round %d0-%d2 to a given precision.
| This will require at least two bits "below" %d2: guard bit and sticky bit.
| These bits can be supplied in another data register (eg, %d6 or %d7).
| fmul can supply these extra bits easily, but for fadd to work, sr64 must
| save the bits that are shifted out.

| input:
|  none
| output:
|  %d0-%d2 = quiet nan
nan:	move.l	#EXT_NAN,%d0
	move.l	#0xc0000000,%d1
	move.l	#0,%d2
	rts

| y = y + x
instr_gen_fadd:
	clr.b	fpsrexc
	movem.l	(%a1),%d3-%d5	| load dst register
	jbsr	fadd
	jbra	ftst_and_save_dst

| y = y - x
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d3-%d5 = y - x
fsub:
	bchg	#EXT_SIGN_BIT,%d0	| negate x and fall through to fadd
	| fall through

| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d0-%d2 = y + x
| TODO: fix this to use the new internal ext float format (unbiased exponents
| etc)
fadd:
	jbsr	check_nan
	beq	0f
	rts
0:
	| put the bigger number in dst (%d3:%d4:%d5)
	| compare exponents
	cmp	%d3,%d0
	blt	0f
	bgt	1f
	| compare fractional parts
	cmp.l	%d4,%d1
	blo	0f
	bhi	1f
	cmp.l	%d5,%d2
	bls	0f
	| swap operands
1:
	exg.l	%d0,%d3
	exg.l	%d1,%d4
	exg.l	%d2,%d5
0:
	| abs(dst) >= abs(src) (excluding sign)
	| check for infinities
	| since dst >= src, if dst is not infinity, src is also not infinity
	btst	#EXT_INF_BIT,%d3
	beq	0f		| if dst is not inf, src cannot be inf
	| dst is inf
	eor.l	%d3,%d0
	btst	#EXT_INF_BIT,%d0
	bne	8f		| src is not inf?
	btst	#EXT_SIGN_BIT,%d0
	bne	nan		| signs are different?

8:	| only dst is inf, or src is inf and has the same sign
	| copy dst (which is inf) to result
	move.l	%d3,%d0
	move.l	%d4,%d1
	move.l	%d5,%d2
	rts

0:
	btst	#EXT_Z_BIT,%d3
	beq	0f
	| if dst = 0, then src = 0
	and.l	%d3,%d0		| -0 only if dst = -0 and src = -0
	bra	fadd_zero
0:
	btst	#EXT_Z_BIT,%d3
	beq	0f
	| src is 0; return dst
	move.l	%d3,%d0
	bra	fadd_end

0:
	| denormalize the smaller operand so its exponent equals the
	| larger operand's exponent
	sub	%d3,%d0
	neg	%d0		| exponent difference
	| shift %d1:%d2 right by %d0 bits
	bsr	sr64_reg
	| sr64 should save our rounding bits so we can use them for rounding
0:
	exg.l	%d3,%d0		| move our exponent to %d0
	| %d3.l still contains a sign bit


	| if signs are the same, add the fractions and normalize
	| otherwise subtract the smaller fraction from the larger and
	| keep the sign of the larger one
	.if 1
	eor.l	%d0,%d3
	btst	#EXT_SIGN_BIT,%d3
	bne	0f	| signs are different?
	.else
	swap	%d0
	swap	%d3
	cmp	%d0,%d3
	bne	0f
	swap	%d3
	.endif
	| signs are the same; add and normalize if carry

	add.l	%d5,%d2
	addx.l	%d4,%d1
	bcc	1f
	| result is supernormal; shift fraction right and increment exponent
	roxr.l	#1,%d1
	roxr.l	#1,%d2
	roxr.l	#1,%d7
	bcc	77f
	or	#1,%d7	| preserve carried bit
77:
	add	#1,%d0	| add 1 to exponent
1:	| TODO: round here
	rts
0:
	| signs are different; subtract src from dst

	neg.l	%d7		| negate rounding bits
	subx.l	%d2,%d5
	subx.l	%d1,%d4
	beq	0f
	bmi	1f
	| shift in our guard digit
	add.l	%d7,%d7
	addx.l	%d5,%d5
	addx.l	%d4,%d4
	sub	#1,%d0
	bra	1f
0:	bset.l	#EXT_Z_BIT,%d0
	bclr.l	#EXT_SIGN_BIT,%d0	| x - x = +0 (except in RM round mode)
fadd_zero:
	move.l	%d4,%d1
	move.l	%d5,%d2
	rts
1:
fadd_end:
	| result is in %d0:%d4:%d5
	| move it to %d0:%d1:%d2
	move.l	%d4,%d1
	move.l	%d5,%d2
9:	|bra	round
	| fall through

| input:
|  %d0-%d2 = value to round
|  %d7 = rounding bits
| output:
|  %d0-%d2 = rounded and normalized value
round:
	tst.l	%d7
	beq	9f	| rounding bits are zero => exact
	bset	#EXC_INEX2_BIT,fpsrexc
	| XXX: for now we assume round-to-nearest (RN) rounding mode
	move.l	%d2,%d3
	and.l	#1,%d3	| get lsb of significand (round to even on 0.5)
	add.l	#0x7fffffff,%d3	| %d3 is either 0.5 or 1 ulp less, depending on lsb of significand
	add.l	%d3,%d7
	move.l	#0,%d3
	addx.l	%d3,%d2
	addx.l	%d3,%d1
	bcc	9f
	| supernormal; shift right and increment exponent
	roxl.l	#1,%d1
	roxl.l	#1,%d2
	add.l	#1,%d0
9:	| normalize and return
	bra	normalize

| y = y * x
instr_gen_fmul:
	movem.l	(%a1),%d3-%d5	| load dst register
	move.l	%a1,-(%sp)
	jbsr	fmul
	move.l	(%sp)+,%a1
	jbra	ftst_and_save_dst

| y = y * x
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d0-%d2 = y * x
fmul:
	| TODO: check for nan, inf, zero, etc
	movem.l	%d0/%d3,-(%sp)
	move.l	%d1,%d0
	move.l	%d2,%d1
	move.l	%d4,%d2
	move.l	%d5,%d3
	jbsr	mulu64c
	move.l	%d2,%d4
	move.l	%d1,%d2
	move.l	%d0,%d1
	movem.l	(%sp)+,%d0/%d3
	add.w	%d3,%d0		| add exponents
	| TODO: check bounds of exponent here
	tst.l	%d1
	bmi	0f
	| high bit of fraction is clear
	add.l	%d4,%d4
	addx.l	%d2,%d2
	addx.l	%d1,%d1
	sub	#1,%d0		| decrement exponent
0:
	| shift exponents back
	lsr.l	#1,%d0
	clr.w	%d3		| clear exponent
	lsr.l	#1,%d3
	| fix the sign bit
	eor.l	%d3,%d0
	rts

| y = y / x (single-precision)
instr_gen_fsgldiv:
	| TODO
	rts

| y = y modulo x (round to nearest)
instr_gen_frem:
	| TODO
	rts

| y = y * 2^int(x)
instr_gen_fscale:
	| TODO
	rts

| y = y * x (single-precision)
instr_gen_fsglmul:
	| TODO
	rts

| y = y - x
instr_gen_fsub:
	movem.l	(%a1),%d3-%d5	| load dst register
	jbsr	fsub
	jbra	ftst_and_save_dst

| c = cos(x)
| y = sin(x)
instr_gen_fsincos:
	| TODO: store cos(x) in c, then store sin(x) in y (in that order)
	| OR store sin(x) in y and skip cos(x) if c == y
	rts

| input:
|  %d0-%d2 = value to normalize
| output:
|  %d0-%d2 = normalized value
| TODO: test and optimize this
normalize:
	move.l	%d1,%d3
	bmi	9f
	or.l	%d2,%d3
	beq	0f

| %d0-%d2 has been tested and found to be denormal
normalize2:
	| it's not zero
	| shift significand left until MSB is 1
	moveq	#1,%d3
1:	sub	%d3,%d0
	add.l	%d2,%d2		| shift it left 1 bit
	addx.l	%d1,%d1
	bpl	1b
	rts
0:	| it's zero
	bset #EXT_Z_BIT,%d0
9:	rts

| convert any format to extended-precision real format
| input:
|  %a0 = pointer to data
|  %d0 = format of data
| output:
|  %d0:%d1:%d2 = extended-precision real value
| note: 
convert_to_ext:
	lsl	#2,%d0
	move.l	(7f,%pc,%d0.w),%a1	| handler table
	jmp	(%a1)
7:
	.long	ctxl
	.long	ctxs
	.long	ctxx
	.long	ctxp
	.long	ctxw
	.long	ctxd
	.long	ctxb

ctxx:
	movem.l	(%a0),%d0-%d2
	clr.w	%d0
	bclr	#15,%d0
	beq	0f
	bset	#EXT_SIGN_BIT,%d0
0:	swap	%d0
	cmp.w	#0x7fff,%d0
	beq	2f		| inf or nan?
	| normal, subnormal, or zero
	sub.w	#EXT_EXP_BIAS,%d0
	bra	normalize
2:	| inf or nan
	| move	#EXT_EXP_MAX-1-EXT_EXP_BIAS,%d0
	move.l	%d1,%d3
	or.l	%d2,%d3
	bne	8f
	| inf
	bset	#EXT_INF_BIT,%d0
	rts
8:	| nan
	bset	#EXT_NAN_BIT,%d0
	rts

| single-precision:
| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| |s|e|e|e|e|e|e|e|e|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|
| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| s eeeeeeee fffffffffffffffffffffff
| shift fraction left 8 bits
| shift exponent right 23 bits (or swap + right 7 bits)
| shift sign right 15 bits, or shift sign left 8 bits after shifting it along
| with the exponent
ctxs:
	move.l	(%a0),%d0
	move.l	%d0,%d1
	lsl.l	#8,%d1		| fraction
	bclr	#31,%d1		| clear lsb of exponent
	clr.w	%d0
	swap	%d0
	bclr	#15,%d0
	beq	0f
	bset	#EXT_SIGN_BIT,%d0
0:	lsr.w	#7,%d0		| right-align the exponent
	beq	1f		| subnormal or zero?
	cmp	#0xff,%d0
	beq	2f		| inf or nan?
	| normal
	bset.l	#31,%d1		| integer bit
	sub.w	#SNG_EXP_BIAS,%d0	| remove exponent bias
	moveq	#0,%d2		| low 32 bits are always 0
	rts

1:	| subnormal or zero
	sub.w	#SNG_EXP_BIAS,%d0	| remove exponent bias
	add.l	%d1,%d1
	bra	normalize32	| subnormal--normalize it
2:	| inf or nan
	moveq	#0,%d2
	| move	#EXT_EXP_MAX-1-EXT_EXP_BIAS,%d0
	tst.l	%d1
	beq	8f
	| nan
	bset.l	#EXT_NAN_BIT,%d0
	rts
8:	| inf
	bset.l	#EXT_INF_BIT,%d0
	rts

| double (1.11.52)
| word 0: seeeeeeeeeeeffff ffffffffffffffff
| word 1: ffffffffffffffff ffffffffffffffff
ctxd:
	movem.l	(%a0),%d0/%d2
	move.l	%d0,%d1

	swap	%d1
| ffffffffffffffff seeeeeeeeeeeffff
	ror.l	#5,%d1
	move	#0xf800,%d3	| mask
	and.w	%d3,%d1		| clear sign/exponent
| 1fffffffffffffff fffff00000000000

	swap	%d2
	ror.l	#5,%d2
	eor.w	%d2,%d1
	and.w	%d3,%d2
	eor.w	%d2,%d1

	clr.w	%d0
	swap	%d0
| 0000000000000000 seeeeeeeeeeeffff
	| TODO: change this to set bit #EXP_SIGN_BIT here
	bclr	#15,%d0
	beq	0f
	bset	#EXT_SIGN_BIT,%d0
0:	lsr.w	#4,%d0		| right-align exponent
	beq	1f		| subnormal/zero?
	cmp	#DBL_EXP_MAX-1,%d0
	beq	2f		| inf/nan?
	| normal
	bset.l	#31,%d1		| explicit integer bit
	sub.w	#DBL_EXP_BIAS,%d0
	rts

1:	| subnormal/zero
	sub	#DBL_EXP_BIAS-1,%d0
	bra	normalize

2:	| inf/nan
	| move	#EXT_EXP_MAX-1-EXT_EXP_BIAS,%d0
	move.l	%d1,%d3
	or.l	%d2,%d3
	beq	8f
	| nan
	bset	#EXT_NAN_BIT,%d0
	rts
8:	| inf
	bset	#EXT_INF_BIT,%d0
	rts
	
ctxp:
	| TODO
	move.l	#0x00020000,%d0
	moveq.l	#0xffffffff,%d1
	move.l	%d1,%d2
	rts
	.if 0
packed decimal format:

+-- sign of mantissa
|  +-- sign of exponent
|  |   +-- used only for +/- infinity or nan's
95 94  |                                   implicit decimal point --+
+-+-+---+-------+-------+-------+-------+-------+-------+-------+   |
|S|s| YY|  EXP2 |  EXP1 |  EXP0 | (EXP3)|0 0 0 0|0 0 0 0|DIGIT16|   |
+-+-+---+-------+-------+-------+-------+-------+-------+-------+ <-+
|DIGIT15|DIGIT14|DIGIT13|DIGIT12|DIGIT11|DIGIT10|DIGIT 9|DIGIT 8|
+-------+-------+-------+-------+-------+-------+-------+-------+
|DIGIT 7|DIGIT 6|DIGIT 5|DIGIT 4|DIGIT 3|DIGIT 2|DIGIT 1|DIGIT 0|
+-------+-------+-------+-------+-------+-------+-------+-------+
	.endif


ctxl:
	moveq.l	#31,%d0
	move.l	(%a0),%d1
	bne	0f
	bra	9f

ctxb:
	moveq.l	#7,%d0
	moveq	#0,%d1
	move.b	(%a0),%d1
	ror.l	#8,%d1		| put the byte in MSB of long
	bne	0f
	bra	9f

ctxw:	| this is probably the most common case
	moveq.l	#15,%d0
	moveq	#0,%d1
	move.w	(%a0),%d1
	swap	%d1		| put the word in MSB of long
	beq	9f

| all integer conversions go here
0:
	| sign
	bpl	2f
	bset	#EXT_SIGN_BIT,%d0	| set sign bit
	neg.l	%d1

| normalize a 32-bit fraction
| fraction is assumed to be tested (eg, tst.l %d1) before this is called
normalize32:
	bmi	8f
	beq	9f
	| normalize the significand
2:	moveq	#1,%d2
1:	sub	%d2,%d0		| --exponent
	add.l	%d1,%d1		| shift fraction left
	bpl	1b		| loop until negative (MSB set)
8:	moveq.l	#0,%d2		| clear lower 32 bits of fraction (it's always 0)
	rts

9:	bset	#EXT_Z_BIT,%d0
	move.w	%d1,%d0		| zero out the exponent and significand
	move.l	%d1,%d2
	rts



| convert extended-precision real format to any format
| input:
|  %d0-%d2 = source
|  %a1 = pointer to destination
|  %d6 = format of destination
| output:
|  none
convert_from_ext:
	lsl	#2,%d6
	move.l	(7f,%pc,%d6.w),%a2	| handler table
	jmp	(%a2)
7:
	.long	cfxl
	.long	cfxs
	.long	cfxx
	.long	cfxp
	.long	cfxw
	.long	cfxd
	.long	cfxb

cfxx:	| extended (1.15.64)
	add.w	#EXT_EXP_BIAS,%d0
	btst	#EXT_SIGN_BIT,%d0
	beq	0f
	bset	#15,%d0
0:	btst	#EXT_NAN_BIT,%d0
	bne	2f
	btst	#EXT_INF_BIT,%d0
	bne	2f
	btst	#EXT_Z_BIT,%d0
	bne	0f
	| normal
9:	swap	%d0
	clr.w	%d0
	movem.l	%d0-%d2,(%a1)
	rts
2:	| inf or nan
	or.w	#0x7fff,%d0
	bra	9b
0:	| zero
	and.w	#0x8000,%d0
	bra	9b

cfxd:	| double-precision (1.11.52)
	| TODO
	move.l	#-1,(%a1)+
	move.l	#-1,(%a1)
	rts

	.long 0xcafebabe
cfxs:	| single-precision (1.8.23)
	| TODO: round to single-precision
	add	#SNG_EXP_BIAS,%d0
	and	#0xff,%d0
	btst	#EXT_SIGN_BIT,%d0
	beq	0f
	bset	#8,%d0
0:	
	moveq	#0,%d3
	move.w	%d0,%d3
	swap	%d3
	rol.l	#7,%d3		| exponent in place

	bclr.l	#31,%d1		| implicit integer bit
	lsr.l	#8,%d1
	or.l	%d1,%d3
	btst	#EXT_NAN_BIT,%d0
	bne	2f
	btst	#EXT_INF_BIT,%d0
	bne	2f
	btst	#EXT_Z_BIT,%d0
	bne	0f
	| normal
9:	move.l	%d3,(%a1)
	rts
2:	| inf or nan
	or.l	#0x7f800000,%d3
	move.l	%d3,(%a1)
	rts
0:	| zero
	and.l	#0x807fffff,%d3
	move.l	%d3,(%a1)
	rts

cfxp:
	| TODO
	rts

cfxl:
	| TODO
	move.l	#-1,(%a1)
	rts

cfxw:
	| TODO
	move.w	#-1,(%a1)
	rts

cfxb:
	| TODO
	move.b	#-1,(%a1)
	rts


| y - x (only set condition flags)
instr_gen_fcmp:
	movem.l	(%a1),%d3-%d5	| load dst register
	| fall through

| y - x (only set condition flags)
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  condition codes are set/cleared as appropriate
fcmp:
	| TODO
	rts

| y = -x
instr_gen_fneg:
	clr.b	fpsrexc
	bchg	#EXT_SIGN_BIT,%d0		| invert the sign
	jbra	ftst_and_save_dst

| y = -x
| input:
|  %d0-%d2 = ext float (x)
| output:
|  %d0-%d2 = ext float with sign inverted (y)
fneg:
	bchg	#EXT_SIGN_BIT,%d0		| invert the sign
	rts

| set fpsrcc for source (x)
| x	fpsrcc	fpsrexc
| +0	Z
| -0	Z N
| +real	none
| -real	N
| +inf	I
| -inf	I N
| +nan	NaN
| -nan	NaN N
| +snan	NaN	SNaN
| -snan	NaN N	SNaN
instr_gen_ftst:
	clr.b	fpsrexc
	| fall through

| input:
|  %d0-%d2 = ext float source in internal format
| output:
|  fpsrcc and fpsrexc flags set according to value of operand
| TODO: clean this up and optimize it
ftst:
	move.l	%d0,%d5		| fp type bits correspond to condition code bits
	swap	%d5
	move.b	%d5,fpsrcc
	btst	#CC_NAN_BIT,%d5
	beq	9f
	| set SNAN if this is a signaling NAN
	btst	#30,%d1			| test quiet bit
	bne	9f
	bset	#EXC_SNAN_BIT,fpsrexc
9:	rts




instr_gen_invalid:
	rts



| test a condition predicate
| input:
|  %d0 = condition predicate
| output:
|  z flag is set if condition is true
|  eg:
|    beq xxx | branch if condition is true
| this could probably be done with some bit voodoo
| each predicate gets 32 bytes to test the condition and return
test_condition:
	move.b	fpsrcc,%d1
	and	#15,%d0
	lsl	#5,%d0
	lea	0f,%a0
	jmp	(%a0,%d0.w)

	.balign 32
0:	| 0 False
	move	#1,%d1
	rts

	.balign 32
	| 1 EQ = Z
	not	%d1
	andi	#CC_Z,%d1
	rts

	.balign 32
	| 2 GT = !NAN x !Z x !N = !(NAN + Z + N)
	andi	#CC_NAN+CC_Z+CC_N,%d1
	rts

	.balign 32
	| 3 GE = Z + !(NAN + N) = Z + !NAN x !N  !!!!TEST THIS!!!!
	btst	#CC_Z_BIT,%d1
	bne	1f
	andi	#CC_NAN+CC_N,%d1
	rts	| same as beq 1f; bra 0b

	.balign 32
	| 4 LT = N x !(NAN + Z) = N x !NAN x !Z
	| = !(!N + (NAN + Z)) = !(!N + NAN + Z)
	| truth table:
	| Z NAN N  LT
	| 0  0  0  0
	| 0  0  1  1
	| 0  1  0  0
	| 0  1  1  0
	| 1  0  0  0
	| 1  0  1  0
	| 1  1  0  0
	| 1  1  1  0
	eori	#CC_N,%d1
	andi	#CC_N+CC_NAN+CC_Z,%d1
	rts

	.balign 32
	| 5 LE = Z + (N x !NAN) = Z + (!!N x !NAN) = Z + !(!N + NAN)  !!!!TEST THIS!!!!
	| truth table:
	| Z NAN N  LE
	| 0  0  0  0
	| 0  0  1  1
	| 0  1  0  0
	| 0  1  1  0
	| 1  0  0  1
	| 1  0  1  1
	| 1  1  0  1
	| 1  1  1  1
	btst	#CC_Z_BIT,%d1
	bne	1f
	eori	#CC_N,%d1
	andi	#CC_N+CC_NAN,%d1
	rts

	.balign 32
	| 6 GL = !(NAN + Z) = !NAN x !Z
	andi	#CC_NAN+CC_Z,%d1
	rts

	.balign 32
	| 7 GLE = !NAN
	andi	#CC_NAN,%d1
	rts

	.balign 32
	| 8 NGLE = NAN
	not	%d1
	andi	#CC_NAN,%d1
	rts

	.balign 32
	| 9 NGL = NAN + Z
	not	%d1
	andi	#CC_NAN+CC_Z,%d1
	rts

	.balign 32
	| a NLE = NAN + !(N + Z) = NAN + !N x !Z
	| truth table:
	| Z NAN N  NLE
	| 0  0  0  1
	| 0  0  1  0
	| 0  1  0  1
	| 0  1  1  1
	| 1  0  0  1
	| 1  0  1  0
	| 1  1  0  0
	| 1  1  1  0
	btst	#CC_NAN_BIT,%d1
	bne	1f
	andi	#CC_N+CC_Z,%d1
	rts

	.balign 32
	| b NLT = NAN + Z + !N
	| truth table:
	| Z NAN N  NLT LT
	| 0  0  0  1  0
	| 0  0  1  0  1
	| 0  1  0  1  0
	| 0  1  1  1  0
	| 1  0  0  1  0
	| 1  0  1  1  0
	| 1  1  0  1  0
	| 1  1  1  1  0
	eori	#CC_NAN+CC_Z,%d1
	andi	#CC_NAN+CC_Z+CC_N,%d1
	rts

	.balign 32
	| c NGE = NAN + (N x !Z) = NAN + !(!N + Z)
	btst	#CC_NAN_BIT,%d1
	bne	1f
	eori	#CC_N,%d1
	andi	#CC_N+CC_Z,%d1
	rts

	.balign 32
	| d NGT = NAN + Z + N
	not	%d1
	andi	#CC_NAN+CC_Z+CC_N,%d1
	rts

	.balign 32
	| e NEQ = !Z
	andi	#CC_Z,%d1
	rts

	.balign 32
1:	| f True
	move	#0,%d1
	rts

	.if 0
mnemonic	predicate	equation
IEEE Nonaware Tests:
EQ		00001		Z
NE		01110		!Z
GT		10010		!(NAN + Z + N) = !NAN x !Z x !N
NGT		11101		NAN + Z + N
GE		10011		Z + !(NAN + N) = Z + !NAN x !N
NGE		11100		NAN + (N x !Z)
LT		10100		N x !(NAN + Z) = N x !NAN x !Z
NLT		11011		NAN + (Z + !N)
LE		10101		Z + (N x !NAN)
NLE		11010		NAN + !(N + Z) = NAN + !N x !Z
GL		10110		!(NAN + Z) = !NAN x !Z
NGL		11001		NAN + Z
GLE		10111		!NAN
NGLE		11000		NAN

IEEE Aware Tests:
EQ		00001		Z
NE		01110		!Z
OGT		00010		!(NAN + Z + N) = !NAN x !Z x !N
ULE		01101		NAN + Z + N
OGE		00011		Z + !(NAN + N) = Z + !NAN x !N
ULT		01100		NAN + (N x !Z)
OLT		00100		N x !(NAN + Z) = N x !NAN x !Z
UGE		01011		NAN + Z + !N
OLE		00101		Z + (N x !NAN)
UGT		01010		NAN + !(N + Z) = NAN + !N x !Z
OGL		00110		!(NAN + Z) = !NAN x !Z
UEQ		01001		NAN + Z
OR		00111		!NAN
UN		01000		NAN

Miscellaneous Tests:
F		00000		False
T		01111		True
SF		10000		False
ST		11111		True
SEQ		10001		Z
SNE		11110		!Z

Note: IEEE aware and IEEE unaware tests are the same except for bit 4 of the predicate. BSUN is set if bit 4 of the predicate is set and NAN is set: BSUN = P[5] x NAN

	.endif

| Scc/DBcc
| Scc: set Dn.b to 0xff if condition is true, or 0 if condition is false
| DBcc: if condition is true, do nothing. otherwise, decrement Dn.w and branch
|       if Dn.w is not -1
| instruction formats:
| Scc:
| 1111iii001eeeeee
| 0000000000pppppp
| DBcc (EA mode = 001):
| 1111iii001001ccc
| 0000000000pppppp
| dddddddddddddddd
| ccc = count register
| ddd... = displacement
instr_scc:
	move	%d7,%d0
	lsr	#3,%d0
	cmp	#1,%d0
	beq	1f
	| fscc
	move	#1,%d0	| size=1 (byte)
	bsr	get_ea
	beq	instr_invalid	| valid?
	move	(%a5)+,%d0
	jbsr	test_condition
	seq	(%a0)
	rts
1:
	| dbcc
	move	(%a5)+,%d0	| condition
	move	(%a5)+,%d3	| displacement
	
	jbsr	test_condition
	beq	1f	| skip if condition is true
	| condition is false
	| decrement Dn.w and branch if Dn.w != -1
	and	#7,%d7
	lsl	#2,%d7
	lea.l	(SAVED_D0,%fp,%d7.w),%a1	| %a1 = pointer to Dn
	sub	#1,(%a1)
	.if 1
	cmp	#-1,(%a1)
	beq	1f
	.else
	bcs	1f	| -1 => borrow (FIXME: is this the correct condition?)
	.endif
	| Dn.w != -1
	| add displacement to PC
	lea.l	(-2,%a5,%d3.w),%a5
1:

	rts

|fbcc:
|1111iii01spppppp
|dddddddddddddddd
|dddddddddddddddd
instr_bcc_w:
	move.w	(%a5)+,%d3	| branch displacement
	ext.l	%d3
	bra	0f

instr_bcc_l:
	move.l	(%a5)+,%d3
	sub	#2,%d3
0:	move	%d7,%d0
	jbsr	test_condition
	bne	1f
	| condition is true
	| branch!
	lea.l	(-2,%a5,%d3.l),%a5
1:	rts

instr_save:
	rts

instr_restore:
	rts

instr_invalid:
	| TODO: send SIGILL signal to process
	rts


fmovecr:
	| move constant ROM value to register
	
	bsr	get_dst
	| get the rom offset
	and	#127,%d6
	mulu	#12,%d6
	lea.l	(constant_rom,%pc,%d6.w),%a0
	| copy value to dst (%a4)
	movem.l	(%a0),%d0-%d2
	move.l	%a4,%a1
	jbra	ftst_and_save_dst

| N.B. constant_rom is stored in the internal extended precision format
constant_rom:
	.long 0x00000001,0xc90fdaa2,0x2168c235  | 00 pi

	| following can store constants for calculating sin(x) and cos(x)
	.long 0x00040000,0x00000000,0x00000000  | 01
	.long 0x00040000,0x00000000,0x00000000  | 02
	.long 0x00040000,0x00000000,0x00000000  | 03
	.long 0x00040000,0x00000000,0x00000000  | 04
	.long 0x00040000,0x00000000,0x00000000  | 05
	.long 0x00040000,0x00000000,0x00000000  | 06
	.long 0x00040000,0x00000000,0x00000000  | 07
	.long 0x00040000,0x00000000,0x00000000  | 08
	.long 0x00040000,0x00000000,0x00000000  | 09
	.long 0x00040000,0x00000000,0x00000000  | 0a

	.long 0x0000fffe,0x9a209a84,0xfbcff798  | 0b log10(2)
	.long 0x00000001,0xadf85458,0xa2bb4a9a  | 0c e
	.long 0x00000000,0xb8aa3b29,0x5c17f0bc  | 0d log2(e)
	.long 0x0000fffe,0xde5bd8a9,0x37287195  | 0e log10(e)
	.long 0x00040000,0x00000000,0x00000000  | 0f 0.0

	| following can store constants for calculating log2(x) and 2^x
	.long 0x00040000,0x00000000,0x00000000  | 10
	.long 0x00040000,0x00000000,0x00000000  | 11
	.long 0x00040000,0x00000000,0x00000000  | 12
	.long 0x00040000,0x00000000,0x00000000  | 13
	.long 0x00040000,0x00000000,0x00000000  | 14
	.long 0x00040000,0x00000000,0x00000000  | 15
	.long 0x00040000,0x00000000,0x00000000  | 16
	.long 0x00040000,0x00000000,0x00000000  | 17
	.long 0x00040000,0x00000000,0x00000000  | 18
	.long 0x00040000,0x00000000,0x00000000  | 19
	.long 0x00040000,0x00000000,0x00000000  | 1a
	.long 0x00040000,0x00000000,0x00000000  | 1b
	.long 0x00040000,0x00000000,0x00000000  | 1c
	.long 0x00040000,0x00000000,0x00000000  | 1d
	.long 0x00040000,0x00000000,0x00000000  | 1e
	.long 0x00040000,0x00000000,0x00000000  | 1f
	.long 0x00040000,0x00000000,0x00000000  | 20
	.long 0x00040000,0x00000000,0x00000000  | 21
	.long 0x00040000,0x00000000,0x00000000  | 22
	.long 0x00040000,0x00000000,0x00000000  | 23
	.long 0x00040000,0x00000000,0x00000000  | 24
	.long 0x00040000,0x00000000,0x00000000  | 25
	.long 0x00040000,0x00000000,0x00000000  | 26
	.long 0x00040000,0x00000000,0x00000000  | 27
	.long 0x00040000,0x00000000,0x00000000  | 28
	.long 0x00040000,0x00000000,0x00000000  | 29
	.long 0x00040000,0x00000000,0x00000000  | 2a
	.long 0x00040000,0x00000000,0x00000000  | 2b
	.long 0x00040000,0x00000000,0x00000000  | 2c
	.long 0x00040000,0x00000000,0x00000000  | 2d
	.long 0x00040000,0x00000000,0x00000000  | 2e
	.long 0x00040000,0x00000000,0x00000000  | 2f

	.long 0x0000ffff,0xb17217f7,0xd1cf79ac  | 30 ln(2)
	.long 0x00000001,0x935d8ddd,0xaaa8ac17  | 31 ln(10)
	.long 0x00000000,0x80000000,0x00000000  | 32 10^0
	.long 0x00000003,0xA0000000,0x00000000  | 33 10^1
	.long 0x00000006,0xC8000000,0x00000000  | 34 10^2
	.long 0x0000000d,0x9C400000,0x00000000  | 35 10^4
	.long 0x0000001a,0xBEBC2000,0x00000000  | 36 10^8
	.long 0x00000035,0x8E1BC9BF,0x04000000  | 37 10^16
	.long 0x0000006a,0x9DC5ADA8,0x2B70B59E  | 38 10^32
	.long 0x000000D4,0xC2781F49,0xFFCFA6D5  | 39 10^64
	.long 0x000001A9,0x93BA47C9,0x80E98CE0  | 3a 10^128
	.long 0x00000352,0xAA7EEBFB,0x9DF9DE8E  | 3b 10^256
	.long 0x000006A4,0xE319A0AE,0xA60E91C7  | 3c 10^512
	.long 0x00000D49,0xC9767586,0x81750C17  | 3d 10^1024
	.long 0x00001A93,0x9E8B3B5D,0xC53D5DE5  | 3e 10^2048
	.long 0x00003526,0xC4605202,0x8A20979B  | 3f 10^4096


	.if 0

nnn=000
cpGEN
1111iii000eeeeee
cccccccccccccccc
xxxxxxxxxxxxxxxx

cc... = coprocessor ID-dependent command word
xx... = optional effective address or coprocessor ID-defined extension words

nnn=001
cpScc
1111iii001eeeeee
0000000000pppppp
xxxxxxxxxxxxxxxx

pppppp = coprocesser ID condition

nnn=001
eeeeee=001???
cpDBcc
1111iii001001rrr
0000000000pppppp
xxxxxxxxxxxxxxxx
dddddddddddddddd

nnn=001
eeeeee=111???
cpTRAPcc
1111iii001111mmm
0000000000pppppp
xxxxxxxxxxxxxxxx
oooooooooooooooo
oooooooooooooooo

mmm = opmode
 mmm > 001 (000 and 001 are reserved for cpScc when mode=111)
ooo... = optional word or long-word operand


nnn=01s
cpBcc
1111iii01spppppp
xxxxxxxxxxxxxxxx
dddddddddddddddd
dddddddddddddddd

nnn=100
cpSAVE
1111iii100eeeeee

nnn=101
cpRESTORE
1111iii101eeeeee

************************************************************************


| fmove (memory/register to register):
| 1111iii000eeeeee
| 0R0sssdddooooooo
|
| sss = source register or format (111 = fmovecr)
| ddd = destination register
| ooooooo = opmode
|  0000000 fmove  rounding precision specified by the floating-point control register
|  1000000 fsmove single-precision rounding specified *
|  1000100 fdmove double-precision rounding specified *
|  * supported by MC68040 only
| 
| R = R/M field
|  0 = source is register
|  1 = source is <ea>
| 
| fmove (register to memory):
| 1111iii000eeeeee
| 011dddssskkkkkkk
| kkkkkkk = k-factor (required only for packed decimal destination)
| 
| fmovecr:
| 1111iii000000000
| 010111dddooooooo
| 
| ooooooo = ROM offset
| 
| ddd = destination format
| sss = source register
| 
| fmovem (system control registers)
| 1111iii000eeeeee
| 10Drrr0000000000
| 
| rrr = register list
|  100 floating-point control register (FPCR)
|  010 floating-point status register (FPSR)
|  001 floating-point instruction address register (FPIAR)
| 
| fmovem (data registers):
| 1111iii000eeeeee
| 11Dmm000rrrrrrrr
|
| D = direction of data transfer
|  0=from memory to fpu
|  1=from fpu to memory
| mm = mode
|  00 static register list, predecrement addressing mode (reg-mem only)
|  01 dynamic register list, predecrement addressing mode (reg-mem only)
|  10 static register list, postincrement or control addressing mode
|  11 dynamic register list, postincrement or control addressing mode
| rrrrrrrr = register list
| 
| 
| list type		register list format
| static, -(An)		FP7	FP6	FP5	FP4	FP3	FP2	FP1	FP0
| static, (An)+,	FP0	FP1	FP2	FP3	FP4	FP5	FP6	FP7
|  or Control
| dynamic		0	r	r	r	0	0	0	0
| 
| dynamic: rrr is data register
| 
| top 3 bits of second instruction word:
| 000 fmove reg-reg
| 001 
| 010 fmove mem-reg OR fmovecr (cr-mem)
| 011 fmove reg-mem
| 100 fmovem memory to system control registers
| 101 fmovem system control registers to memory
| 110 fmovem memory to data registers
| 111 fmovem data registers to memory



fsincos:
1111iii000eeeeee
0r0sssddd0110ccc
ddd = destination register FPs
ccc = destination register FPc


most other instructions:
1111iii000eeeeee
0r0sssdddooooooo

eeeeee = effective address (3 bits mode, 3 bits register)
r = R/M
sss = source specifier
ddd = destination register
ooooooo = opmode



R/M field:
 0 = register to register
 1 = <ea> to register

Destination register field:
specifies the destination FP data register

Effective address field:
mode	register	addressing mode
000	Dn		Dn
001	An		An
010	An		(An)
011	An		(An)+
100	An		-(An)
101	An		(d16,An)
110	An		(d8,An,Xn)
110	An		(bd,An,Xn)		not on 68000
110	An		([bd,An,Xn],od)		not on 68000
110	An		([bd,An],Xn,od)		not on 68000
111	000		(xxx).W
111	001		(xxx).L
111	010		(d16,PC)
111	011		(d8,PC,Xn)
111	011		(bd,PC,Xn)		not on 68000
111	011		([bd,PC,Xn],od)		not on 68000
111	011		([bd,PC],Xn,od)		not on 68000
111	100		#<data>

Predicates:
mnemonic definition			predicate	BSUN bit set?

IEEE Nonaware Tests
EQ       Equal				000001		No
NE       Not Equal			001110		No
GT       Greater Than			010010		Yes
NGT      Not Greater Than		011101		Yes
GE       Greater Than or Equal		010011		Yes
NGE      Not Greater Than or Equal	011100		Yes
LT       Less Than			010100		Yes
NLT      Not Less Than			011011		Yes
LE       Less Than or Equal		010101		Yes
NLE      Not Less Than or Equal		011010		Yes
GL       Greater or Less Than		010110		Yes
NGL      Not Greater or Less Than	011001		Yes
GLE      Greater, Less or Equal		010111		Yes
NGLE     Not Greater, Less or Equal	011000		Yes

IEEE Aware Tests
EQ       Equal				000001		No
NE       Not Equal			001110		No
OGT      Ordered Greater Than		000010		No
ULE      Unordered or Less or Equal	001101		No
OGE      Ordered Greater Than or Equal	000011		No
ULT      Unordered or Less Than		001100		No
OLT      Ordered Less Than		000100		No
UGE      Unordered or Greater or Equal	001011		No
OLE      Ordered Less Than or Equal	000101		No
UGT      Unordered or Greater Than	001010		No
OGL      Ordered Greater or Less Than	000110		No
UEQ      Unordered or Equal		001001		No
OR       Ordered			000111		No
UN       Unordered			001000		No

Miscellaneous Tests
F        False				000000		No
T        True				001111		No
SF       Signaling False		010000		Yes
ST       Signaling True			011111		Yes
SEQ      Signaling Equal		010001		Yes
SNE      Signaling Not Equal		011110		Yes

	.endif
