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

.equiv	FPCC_NAN_BIT, 0
.equiv	FPCC_I_BIT,   1
.equiv	FPCC_Z_BIT,   2
.equiv	FPCC_N_BIT,   3
.equiv	FPCC_NAN, (1<<FPCC_NAN_BIT)
.equiv	FPCC_I,   (1<<FPCC_I_BIT)
.equiv	FPCC_Z,   (1<<FPCC_Z_BIT)
.equiv	FPCC_N,   (1<<FPCC_N_BIT)

.equiv	FPCRMODE_RND_MASK,  0x30
.equiv	FPCRMODE_PREC_MASK, 0xc0

| these bits apply to the FPCR ENABLE byte and FPSR EXC byte
.equiv	FPEXC_INEX1_BIT, 0
.equiv	FPEXC_INEX2_BIT, 1
.equiv	FPEXC_DZ_BIT,    2
.equiv	FPEXC_UNFL_BIT,  3
.equiv	FPEXC_OVFL_BIT,  4
.equiv	FPEXC_OPERR_BIT, 5
.equiv	FPEXC_SNAN_BIT,  6
.equiv	FPEXC_BSUN_BIT,  7

.equiv	FPSRAEXC_INEX_BIT, 3	| INEX1 + INEX2 + OVFL
.equiv	FPSRAEXC_DZ_BIT,   4	| DZ
.equiv	FPSRAEXC_UNFL_BIT, 5	| UNFL L INEX2
.equiv	FPSRAEXC_OVFL_BIT, 6	| OVFL
.equiv	FPSRAEXC_IOP_BIT,  7	| SNAN + OPERR

| format of extended-precision float register:
| seee eeee eeee eeee 0000 0000 0000 0000
| ffff ffff ffff ffff ffff ffff ffff ffff
| ffff ffff ffff ffff ffff ffff ffff ffff
| 
| s=sign
| e=exponent
| f=fraction (with explicit integer bit)
.equiv EXT_SIGN_OFFSET, 0
.equiv EXT_SIGN_BIT,   31
.equiv EXT_EXP_OFFSET,  0
.equiv EXT_EXP_BIT,    16
.equiv EXT_FRAC_OFFSET, 1 | first long that contains the fraction
.equiv EXT_EXP_BIAS, 16383

.text


	| offsets from %fp
	.equ SAVED_FP,0
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
	move.l	%a0,(SAVED_A7,%fp)
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
	move.l	(SAVED_A7,%sp),%a0
	move.l	%a0,%usp
	unlk	%fp
	movem.l	(%sp)+,%d0-%d7/%a0-%a6
	lea.l	(4,%sp),%sp
	
	| 6. return from trap
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

| input:
|  %d0 = format (L/S/X/P/W/D/B)
| output:
|  %d0 = size of format
get_format_size:
	move.b	(format_sizes,%pc,%d0.w),%d0
	ext.w	%d0
	rts
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
.equiv EA_WRITABLE,  1
.equiv EA_MEMORY,    2
.equiv EA_PREDEC,    4	| this can be repeated safely
.equiv EA_POSTINC,   8	| this can be repeated safely
.equiv EA_DATAREG,  16
.equiv EA_ADDRREG,  32
.equiv EA_PREPOST, EA_PREDEC+EA_POSTINC
get_ea:
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
	move	(%a5)+,%d0
	lea.l	(-2,%a5,%d0.w),%a0
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
	add.w	%a0,%d2
	lea.l	(%a3,%a0.w),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d1,1f	| %d1 == 4?
	| #<data> -- tested, works
	| TODO
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
	move.l	#0,%a0
	move	#EA_MEMORY,%d0
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
	| Dn -- tested, works
	cmp	#4,%d0
	ble	0f
	move.l	#0,%a3	| invalid size for Dn
	rts
	
0:
	| add (4-size) to offset into register (byte: 3, word: 2, long: 0)
	subq	#4,%d0
	sub	%d0,%d1
	
	lea.l	(SAVED_D0,%fp,%d1.w),%a3
	move	#EA_DATAREG+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 1?
	| An
	cmp	#4,%d0
	beq	0f
	move.l	#0,%a3	| invalid size for An
	rts

0:
	lea.l	(SAVED_A0,%fp,%d1.w),%a3
	move	#EA_ADDRREG+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 2?
	| (An) -- tested, works
	move.l	(SAVED_A0,%fp,%d1.w),%a3
	move	#EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 3?
	| (An)+ -- tested, works
	lea.l	(SAVED_A0,%fp,%d1.w),%a3
	cmp	#4*7,%d1	| is this A7?
	bne	0f
	cmp	#1,%d0		| is the size 1?
	bne	0f
	add.l	%d0,(%a3)	| (A7)+ with size=1
	| XXX: is a byte in the upper or lower half of a word on the stack?
0:
	move.l	(%a3),%a0
	ext.l	%d0
	add.l	%d0,(%a3)
	move	#EA_POSTINC+EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 4?
	| -(An) -- tested, works
	lea.l	(SAVED_A0,%fp,%d1.w),%a3
	ext.l	%d0
	sub.l	%d0,(%a3)
	move.l	(%a3),%a0
	cmp	#4*7,%d1
	bne	0f
	cmp	#1,%d0
	bne	0f
	sub.l	%d0,(%a3)	| -(A7) with size=1
	| XXX: is a byte in the upper or lower half of a word on the stack?
0:
	move	#EA_PREDEC+EA_MEMORY+EA_WRITABLE,%d0
	rts

1:	dbra	%d2,1f	| %d2 == 5?
	| (d16,An) -- tested, works
	move.l	(SAVED_A0,%fp,%d1.w),%a0
	move	(%a5)+,%d0
	lea.l	(%a0,%d0.w),%a0
	move	#EA_MEMORY+EA_WRITABLE,%d0
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
	rts
	

| input:
|  %d7 = first instruction word
|  %d6 = second instruction word
|  %d0 = size of operand
| output:
|  %a0 = pointer to source
get_src:
	btst	#14,%d6		| R/M bit
	bne	get_ea		| get source from memory

	| get the source fp register
	move	%d6,%d1
	.if fpregsize == 16
	lsr	#10-4,%d1	| multiplied by 16
	and	#7*16,%d1
	.elseif fpregsize == 12
	lsr	#10-3,%d1
	and	#7*8,%d0
	move	%d1,%d0
	lsr	#1,%d1
	add	%d0,%d1		| multiplied by 12
	.else
	.error
	.endif
	lea.l	fpregs,%a0
	lea.l	(%a0,%d1.w),%a0
	|move	#FORMAT_REAL_EXT,%d2
	rts

| input:
|  %d6 = second instruction word
| output:
|  %a4 = pointer to destination fp data register
get_dst:
	| get the destination fp register
	move	%d6,%d0
	.if fpregsize == 16
	lsr	#7-4,%d0	| multiplied by 16
	and	#7*16,%d0
	.elseif fpregsize == 12
	lsr	#7-3,%d0
	and	#7*8,%d0
	move	%d0,%d1
	lsr	#1,%d0
	add	%d1,%d0		| multiplied by 12
	.else
	.error
	.endif
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

| copy extended-precision floating-point value from src to dst
| input:
|  %a0 = src
|  %a1 = dst
| output:
|  %a0 and %a1 point to the next pair of values
copyfp:
	move.l	(%a0)+,(%a1)+
	move.l	(%a0)+,(%a1)+
	move.l	(%a0)+,(%a1)+
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
	
	| regular general instruction
	bsr	get_dst
	bsr	get_format
	move	%d0,%d3		| save format
	
	.if 0
	bsr	get_format_size
	.else
	move.b	(format_sizes,%pc,%d0.w),%d0	| get the format size
	ext.w	%d0
	.endif
	bsr	get_src
	| TODO: validate src here (test %a3 for zero)
	move	%d3,%d0		| restore format
	jbsr	convert_to_ext
	move	%d6,%d5
	and	#127,%d5
	lsl	#2,%d5
	move.l	%a4,%a1
	
	| %d0-%d2 = source extended-precision value
	| %a1 = destination register
	move.l	(7f,%pc,%d5.w),%a3
	| movem.l	(%a1),%d3-%d5	| pre-load destination value
	jmp	(%a3)
/*
 * All arithmetic instruction handlers take the source value in %d0-%d2.
 * The low-level arithmetic routines generally accept values in %d0-%d2 (source)
 * and %d3-%d5 (destination) which simplifies call sequences.
 * 
 * N.B. All arithmetic routines, with the exception of fcmp for some reason,
 * set the condition codes the same way. fcmp always clears the I condition
 * code since it's not used in any conditional predicate. If we are willing to
 * sacrifice a slight bit of accuracy here we can set condition codes in a
 * common place (eg, at the end of instr_gen).
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



fmovecr:	| fmovecr
	| move constant ROM value to register
	
	bsr	get_dst
	| get the rom offset
	move	%d6,%d0
	and	#127,%d0
	lsl	#2,%d0
	lea.l	(constant_rom,%pc,%d0.w),%a0
	move.l	%a4,%a1
	| %a0 = src (constant rom)
	| %a1 = dst
	jbra	copyfp

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
	move.b	(format_sizes,%pc,%d0.w),%d0
	ext.w	%d0
	move	%d0,-(%sp)
	bsr	get_ea		| get destination ea
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
	
	move	%d2,%d0	| format
	movem.l	(%a4),%d0-%d2	| get register
	jbsr	convert_from_ext
	jbsr	ftst
	
	rts

| input:
|  %d0 = EA type
|  %d1 = format size
| output:
|  zero is set if valid
| %d3 is clobbered
ea_is_valid_dst:
	btst	#EA_WRITABLE,%d0
	beq	1f
	| fall through (a valid dst is also a valid src--most of the time)

| input:
|  %d0 = EA type
|  %d1 = format size
| output:
|  zero is set if valid
| %d3 might be clobbered
ea_is_valid_src:
	btst	#EA_ADDRREG,%d0		| address registers are not allowed as src or dst
	bne	2f
	btst	#EA_MEMORY,%d0
	bne	0f
	| it's a data register; verify that format size is <= 4
	cmp	#4,%d1
	bgt	0f
1:	move	#1,%d3		| invalid
	rts
0:	move	#0,%d3		| valid
2:	rts

| input:
|  %d0-%d2 = value to save
|  %a1 = destination
| output:
|  condition codes are set
| this routine is called by nearly all arithmetic instruction handlers to save
| the current value to destination and then set condition codes.
save_dst_and_ftst:
	swap	%d0	| fix sign/exponent word
	clr.w	%d0
	movem.l	%d0-%d2,(%a1)
	swap	%d0
	jbra	ftst

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
	jbra	save_dst_and_ftst

| input:
|  %a1 = dst
| output:
|  set z flag if dst is nan
|  %d0 is clobbered
is_dst_nan:
	move	(%a1),%d0
	not	%d0
	add	%d0,%d0			| isolate the exponent field (remove the sign)
	bne	2f			| it's not nan (z flag is clear)
	| it's infinity or nan
	move.l	(4,%a1),%d0
	add	%d0,%d0			| ignore MSB (explicit integer bit)
	or.l	(8,%a1),%d0
	not.l	%d0	| z flag is set if nan
2:	rts
	
| check and try to handle nan
| input:
|  %a0 = src
|  %a1 = dst
| output:
|  zero is set if nan handled
|  %d0 is clobbered
| 
| src  dst  result
| nan  nan  nan (from dst)
| n    nan  nan (from dst)
| nan  n    nan (from src)
| 
| signaling nan is converted to quiet nan
check_nan:
	bsr	is_dst_nan
	beq	0f

	| test src
	exg.l	%a0,%a1
	bsr	is_dst_nan
	bne	9f

	| src is nan
	| copy src (%a1) to dst (%a0)
	move.l	(%a1)+,(%a0)+
	move.l	(%a1)+,(%a0)+
	move.l	(%a1)+,(%a0)+
	lea	(-12,%a0),%a0
	|lea	(-12,%a1),%a1
	exg.l	%a0,%a1

0:	| dst is nan
	bset.l	#30,(4,%a1)	| set quiet nan bit
	.if 0
	lea.l	(4,%sp),%sp	| return from our caller
	.else
	moveq	#0,%d0	| set z flag
	.endif
9:	rts

| y = integer part of x
instr_gen_fint:
	rts

| y = sinh(x)
instr_gen_fsinh:
	rts

| y = integer part of x, round to zero
instr_gen_fintrz:
	rts

| y = sqrt(x)
instr_gen_fsqrt:
	rts

| y = ln(x+1)
instr_gen_flognp1:
	rts

| y = e^(x-1)
instr_gen_fetoxm1:
	rts

| y = tanh(x)
instr_gen_ftanh:
	rts

| y = atan(x)
instr_gen_fatan:
	rts

| y = asin(x)
instr_gen_fasin:
	rts

| y = atanh(x)
instr_gen_fatanh:
	rts

| y = sin(x)
instr_gen_fsin:
	rts

| y = tan(x)
instr_gen_ftan:
	rts

| y = e^x
instr_gen_fetox:
	rts

| y = 2^x
instr_gen_ftwotox:
	rts

| y = 10^x
instr_gen_ftentox:
	rts

| y = ln(x)
instr_gen_flogn:
	rts

| y = log10(x)
instr_gen_flog10:
	rts

| y = log2(x)
instr_gen_flog2:
	rts

| y = abs(x)
instr_gen_fabs:
	clr.b	fpsrexc
	bsr	fabs
	jbra	save_dst_and_ftst

| input:
|  %d0-%d2 = ext float
| output:
|  %d0-%d2 = ext float with its sign cleared
fabs:
	bclr	#31,%d0
	rts

| y = cosh(x)
instr_gen_fcosh:
	rts

| y = acos(x)
instr_gen_facos:
	rts

| y = cos(x)
instr_gen_fcos:
	rts

| y = getexp(x)
instr_gen_fgetexp:
	rts

| y = getman(x)
instr_gen_fgetman:
	rts

| y = y / x
instr_gen_fdiv:
	movem.l	(%a1),%d3-%d5	| get destination (y)
	jbsr	fdiv
	jbra	save_dst_and_ftst

| y = y / x
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d3-%d5 = y / x
fdiv:
	| TODO
	rts

| y = y modulo x (round to zero)
instr_gen_fmod:
	rts

	.global sr64
	.global sl64
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
	move.l	(4,%sp),%d1
	move.l	(8,%sp),%d2
	move.w	(12,%sp),%d0
	
| shift right a 64-bit number
| input:
|  %d1:%d2 = 64-bit number (%d1 is upper 32 bits)
|  %d0.w = shift amount (unsigned)
| output:
|  %d1:%d2, shifted
sr64_reg:
	cmp	#32,%d0
	bhs	5f
	ror.l	%d0,%d1
	lsr.l	%d0,%d2		| 00..xx (upper bits cleared)
	
	move.l	%d3,-(%sp)
	
	| compute masks
	moveq	#-1,%d3
	lsr.l	%d0,%d3		| 00..11 (lower bits)
	move.l	%d3,%d0
	not.l	%d0		| 11..00 (upper bits)
	
	and.l	%d1,%d0		| only upper bits from %d1
	or.l	%d0,%d2		| put upper bits from %d1 into %d2
	and.l	%d3,%d1		| clear upper bits in %d1
	
	move.l	(%sp)+,%d3
	rts

	| shift amount is >= 32
5:
	cmp	#64,%d0
	bhs	6f
	sub	#32,%d0
	move.l	%d1,%d2
	lsr.l	%d0,%d2
	moveq	#0,%d1
	rts

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
sl64_reg:
	cmp	#32,%d0
	bhs	5f
	rol.l	%d0,%d2
	lsl.l	%d0,%d1		| xx..00 (lower bits cleared)
	
	move.l	%d3,-(%sp)
	
	.if 0
	| compute masks
	moveq	#-1,%d3		| mask
	lsl.l	%d0,%d3		| 11..00 (upper bits)
	move.l	%d3,%d0
	not.l	%d0		| 00..11 (lower bits)
	
	and.l	%d2,%d0		| only lower bits from %d2
	or.l	%d0,%d1		| put lower bits from %d2 into %d1
	and.l	%d3,%d2		| clear lower bits in %d2
	.else
	| awesome optimized code from Samuel Stearley (thanks man!)
	| TODO: adapt this for sr64

	| compute masks
	moveq   #-1,%d3
	bclr.l  %d0,%d3
	addq.l  #1,%d3     | 11..00 (upper bits)

	eor.l  %d2,%d1     | fill in the lower bits and perturb the upper bits
	and.l  %d3,%d2     | clear the lower bits
	eor.l  %d2,%d1     | un-perturb the upper bits.
	.endif
	
	move.l	(%sp)+,%d3
	rts

	| shift amount is >= 32
5:
	cmp	#64,%d0
	bhs	6f
	sub	#32,%d0
	move.l	%d2,%d1
	lsl.l	%d0,%d1
	moveq	#0,%d2
	rts

	| shift amount is >= 64
6:
	moveq.l	#0,%d1
	move.l	%d1,%d2
	rts

| TODO: write a routine to get types of src and dst (stored in %d0-%d2 and
| %d3-%d5) and return types in %d6 or %d7 (leave %d0-%d5 untouched) so it can be
| used as an index into jump tables in any arith operation that wants to do so.
| types: zero, in-range, inf, nan
| each type fits in 2 bits
| this will give a 4x4 (16 entry) jump table


| y = y + x
instr_gen_fadd:
	movem.l	(%a1),%d3-%d5	| get destination (y)
	jbsr	fadd
	jbra	save_dst_and_ftst

| y = y - x
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d3-%d5 = y - x
fsub:
	bchg	#15,%d0		| negate x and fall through to fadd
	| fall through

| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d0-%d2 = y + x
fadd:
	jbsr	check_nan
	bne	0f
	rts
0:
	| TODO: check for infinities
	swap	%d0
	swap	%d3
	add.l	%d0,%d0		| shift the sign bit to bit 16
	add.l	%d3,%d3
	lsl.l	#1,%d0
	lsl.l	#1,%d3
	and	#0x7fff,%d0
	and	#0x7fff,%d3
	| src = %d0:%d1:%d2
	| dst = %d3:%d4:%d5

	| put the bigger number in dst (%d3:%d4:%d5)
	| compare exponents
	cmp	%d3,%d0
	blt	0f
	| compare fractional parts
	cmp.l	%d4,%d1
	blt	0f
	cmp.l	%d5,%d2
	ble	0f
	| swap operands
	exg.l	%d0,%d3
	exg.l	%d1,%d4
	exg.l	%d2,%d5
0:
	| dst >= src (excluding sign)
	| denormalize the smaller operand so its exponent equals the
	| larger operand's exponent
	sub	%d3,%d0
	neg	%d0
	lsr	#1,%d0		| exponent difference

	| shift %d1:%d2 right by %d0 bits
	bsr	sr64_reg


	| TODO: if signs are the same, add the fractions and normalize
	| otherwise subtract the smaller fraction from the larger and
	| keep the sign of the larger one
	swap	%d0
	swap	%d3
	cmp	%d0,%d3
	bne	0f
	| signs are the same; add and normalize if carry
	swap	%d3

	add.l	%d2,%d5
	addx.l	%d1,%d4
	bcc	1f
	| result is supernormal; shift fraction right and increment exponent
	roxr.l	#1,%d4
	roxr.l	#1,%d5
	add	#2,%d3	| add 1 to exponent (%d3 is still shifted)
	cmp	#0xfffe,%d3	| test for overflow
	bne	1f
	| set overflow bit in fpsrexc
	bset	#FPEXC_OVFL_BIT,fpsrexc
	| set dst to infinity
	moveq.l	#0,%d4
	move.l	%d4,%d5
	bra	1f
0:
	| signs are different; subtract src from dst

	sub.l	%d2,%d5
	subx.l	%d1,%d4
	bcc	1f
	| result needs to be normalized
	| TODO: normalize result
	nop
1:
	lsr.l	#1,%d3		| unshift dst exponent
	| result is in %d3:%d4:%d5
	| move it to %d0:%d1:%d2
	move.l	%d3,%d0
	move.l	%d4,%d1
	move.l	%d5,%d2
	rts

| y = y * x
instr_gen_fmul:
	movem.l	(%a1),%d3-%d5	| get destination (y)
	jbsr	fmul
	jbra	save_dst_and_ftst

| y = y * x
| input:
|  %d0-%d2 = x
|  %d3-%d5 = y
| output:
|  %d0-%d2 = y * x
fmul:
	| TODO
	rts

| y = y / x (single-precision)
instr_gen_fsgldiv:
	rts

| y = y modulo x (round to nearest)
instr_gen_frem:
	rts

| y = y * 2^int(x)
instr_gen_fscale:
	rts

| y = y * x (single-precision)
instr_gen_fsglmul:
	rts

| y = y - x
instr_gen_fsub:
	movem.l	(%a1),%d3-%d5	| get destination (y)
	jbsr	fsub
	jbra	save_dst_and_ftst

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
	add.l	%d0,%d0		| push sign bit out of exponent field
	move.l	%d1,%d3
	bmi	9f
	or.l	%d2,%d3
	beq	0f

	| it's not zero
	| shift significand left until MSB is 1 OR exponent is minimum
1:	sub	#1,%d0
	bcs	2f
	lsl.l	#1,%d2
	roxl.l	#1,%d1
	dbmi	%d0,1b
1:	lsr.l	#1,%d0

	rts
2:	| minimum exponent (subnormal)
	clr.w	%d0
	bra	1b
0:	| it's zero
	move	%d1,%d0	| zero exponent
	lsr.l	#1,%d0	| preserve sign
9:	rts

| convert any format to extended-precision real format
| input:
|  %a0 = pointer to data
|  %d0 = format of data
| output:
|  %d0:%d1:%d2 = extended-precision real value
| note: 
| TODO: change this so it produces its result in %d0-%d2 instead of in (%a1)
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
	movem.l	(%a0),%d3-%d5
	| TODO: normalize it
	rts

| single-precision:
| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| |s|e|e|e|e|e|e|e|e|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|f|
| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
ctxs:
	move.l	(%a0),%d0
	swap	%d0		| single with words swapped
	move.l	%d0,%d1
	not	%d1
	and	#0x7f80,%d1	| exponent with bias
	bne	0f
	| inf or nan
	move	%d0,%d1
	or	#0x7fff,%d1	| expand the exponent field
	and	#0xff,%d0	| mask out the fraction
	swap	%d0
	lsl.l	#8,%d0
	
	moveq	#0,%d3
	move	%d1,%d3		| sign and exponent
	swap	%d3
	move.l	%d0,%d4		| fraction
	clr.l	%d5
	rts
	
0:	| number is in range
	moveq	#0,%d5		| we don't use the last long
	move	%d0,%d2
	and	#0x8000,%d2	| sign bit
	move	%d0,%d1
	and	#0x7f,%d0	| mask out fraction
	bset	#7,%d0		| explicit integer bit
	lsr	#7,%d1
	and	#0xff,%d1
	bne	0f
	| subnormal
	addq	#1,%d1		| ++exponent
	bclr	#7,%d0		| no implicit integer bit
	tst.l	%d0
	beq	8f		| zero?
0:
	swap	%d0		| put fraction field back in place
	rol.l	#8,%d0
	sub	#127,%d1
	
	| normalize the fraction
	bra	7f

8:	| it's zero
	move	%d2,(%a1)	| sign bit
	clr.l	(4,%a1)
	lea.l	(%a1),%a0
	rts

ctxd:
	move.l	(%a0),%d0	| exponent and upper 20 bits of fraction
	move.l	(4,%a0),%d4	| lower 32 bits of fraction
	swap	%d0
	swap	%d4
	move.l	%d0,%d2
	move.l	%d0,%d3
	and	#0x000f,%d3	| mask out the fraction
	bset	#4,%d3		| explicit integer bit
	ror.l	#5,%d3		| upper 21 bits of fraction
	
	ror.l	#5,%d4
	move	%d3,%d1
	and	#0x07ff,%d1
	or	%d1,%d3		| %d3 = upper 32 bits of fraction
	and	#0xf800,%d4	| %d4 = lower 21 bits of fraction
	
	move.l	%d0,%d1
	not	%d1
	and	#0x7ff0,%d1
	bne	0f
	| inf or nan
	or	#0x7fff,%d0	| expand exponent
	move	%d0,(%a1)+
	clr	(%a1)+
	move.l	%d3,(%a1)+
	move.l	%d4,(%a1)
	lea.l	(-8,%a1),%a0
	rts
0:
	| number is in range
	move	%d0,%d1
	move	%d0,%d2
	and	#0x8000,%d2	| sign bit
	and	#0x7ff0,%d1
	lsr	#4,%d1		| exponent with bias
	bne	3f
	| subnormal or zero
	addq	#1,%d1		| ++exponent
	bclr.l	#31,%d3		| no implicit integer bit
	tst.l	%d3
	bne	0f
	tst.l	%d4
	beq	8f
0:
	| subnormal
	| normalize the fraction
	tst.l	%d3
	bra	1f
0:
	subq	#1,%d1		| --exponent
	| shift left
	lsl.l	#1,%d4
	roxl.l	#1,%d3
1:
	bpl	0b		| loop until negative (MSB set)

3:	| normal
	add	#EXT_EXP_BIAS-1023,%d1
	or	%d1,%d2
	move	%d2,(%a1)+
	clr	(%a1)+
	move.l	%d3,(%a1)+
	move.l	%d4,(%a1)
	lea.l	(-8,%a1),%a0
	rts

8:	| it's zero
	move	%d2,(%a1)+
	clr	(%a1)+
	clr.l	(%a1)+
	clr.l	(%a1)
	lea.l	(-8,%a1),%a0
	rts

ctxp:
	| TODO
	move.l	#0x7fffffff,(%a1)+
	move.l	#0xffffffff,(%a1)+
	move.l	#0xffffffff,(%a1)+
	lea.l	(-12,%a1),%a0
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
	move.l	(%a0),%d1
	move.l	#EXT_EXP_BIAS+31,%d0
	bra	0f

ctxb:
	moveq	#0,%d1
	move.b	(%a0),%d1
	ror.l	#8,%d1		| put the byte in MSB of long
	move.l	#EXT_EXP_BIAS+7,%d0
	bra	0f

ctxw:	| this is probably the most common case
	moveq	#0,%d1
	move.w	(%a0),%d1
	swap	%d1		| put the word in MSB of long
	move.l	#EXT_EXP_BIAS+15,%d0

| all integer conversions go here
0:	moveq.l	#0,%d2	| clear lower 32 bits of fraction (it's always 0)
	
	| sign
	tst.l	%d1
	beq	9f	| zero -- we're all set
	bpl	7f
	bset	#15,%d0	| set sign bit
	neg.l	%d1

	| normalize the significand
	| good entry point for all formats with a significand
	| that fits in 32 bits
7:
	|tst.l	%d1
	bra	1f
0:	subq	#1,%d0		| --exponent
	lsl.l	#1,%d1		| shift fraction left
1:
	bpl	0b		| loop until negative (MSB set)
9:	rts



| convert extended-precision real format to any format
| input:
|  %d0-%d2 = source
|  %a0 = pointer to destination
|  %d0 = format of destination
| output:
|  none
convert_from_ext:
	lsl	#2,%d0
	move.l	(7f,%pc,%d0.w),%a2	| handler table
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
	| copy it to %a0
	swap	%d0
	clr.w	%d0
	movem.l	%d0-%d2,(%a0)
	rts

cfxd:	| double-precision (1.11.52)
	| TODO
	move.l	#-1,(%a0)+
	move.l	#-1,(%a0)
	rts

cfxs:	| single-precision (1.8.23)
	| TODO
	move.l	#-1,(%a0)
	rts

cfxp:
	| TODO
	rts

cfxl:
	| TODO
	move.l	#-1,(%a0)
	rts

cfxw:
	| TODO
	move.w	#-1,(%a0)
	rts

cfxb:
	| TODO
	move.b	#-1,(%a0)
	rts


| y - x (only set condition flags)
instr_gen_fcmp:
	movem.l	(%a1),%d3-%d5	| get destination
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
	bsr	fneg
	jbra	save_dst_and_ftst

| y = -x
| input:
|  %d0-%d2 = ext float (x)
| output:
|  %d0-%d2 = ext float with sign inverted (y)
fneg:
	bchg	#15,%d0		| invert the sign
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
|  %d0-%d2 = ext float source
| output:
|  fpsrcc and fpsrexc flags set according to value of operand
| TODO: clean this up and optimize it
ftst:
	moveq	#0,%d5		| fpsrcc

	move.l	%d0,%d3
	bset	#15,%d3
	beq	0f
	| it's negative
	bset	#FPCC_N_BIT,%d5
0:
	not	%d3
	beq	1f		| exponent == 0x7fff?

	| it's in range; test for zero
	move.l	%d1,%d3		| upper 32 bits of fraction
	or.l	%d2,%d3		| lower 32 bits of fraction

	bne	9f
	| it's zero
	bset	#FPCC_Z_BIT,%d5
9:	move.b	%d5,fpsrcc
	rts

1:	| it's infinity or nan
	move.l	%d1,%d3
	add.l	%d3,%d3			| ignore MSB (explicit integer bit)
	or.l	%d2,%d3
	bne	0f
	| it's infinity
	bset	#FPCC_I_BIT,%d5
	bra	9f	| end
0:	| it's nan
	bset	#FPCC_NAN_BIT,%d5
	| set SNAN in fpsrexc byte if this is a signaling NAN 
	btst	#30,%d1			| test signal bit
	bne	9f
	bset	#FPEXC_SNAN_BIT,fpsrexc
9:	move.b	%d5,fpsrcc
	rts



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
	andi	#FPCC_Z,%d1
	rts

	.balign 32
	| 2 GT = !NAN x !Z x !N = !(NAN + Z + N)
	andi	#FPCC_NAN+FPCC_Z+FPCC_N,%d1
	rts

	.balign 32
	| 3 GE = Z + !(NAN + N) = Z + !NAN x !N  !!!!TEST THIS!!!!
	btst	#FPCC_Z_BIT,%d1
	bne	1f
	andi	#FPCC_NAN+FPCC_N,%d1
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
	eori	#FPCC_N,%d1
	andi	#FPCC_N+FPCC_NAN+FPCC_Z,%d1
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
	btst	#FPCC_Z_BIT,%d1
	bne	1f
	eori	#FPCC_N,%d1
	andi	#FPCC_N+FPCC_NAN,%d1
	rts

	.balign 32
	| 6 GL = !(NAN + Z) = !NAN x !Z
	andi	#FPCC_NAN+FPCC_Z,%d1
	rts

	.balign 32
	| 7 GLE = !NAN
	andi	#FPCC_NAN,%d1
	rts

	.balign 32
	| 8 NGLE = NAN
	not	%d1
	andi	#FPCC_NAN,%d1
	rts

	.balign 32
	| 9 NGL = NAN + Z
	not	%d1
	andi	#FPCC_NAN+FPCC_Z,%d1
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
	btst	#FPCC_NAN_BIT,%d1
	bne	1f
	andi	#FPCC_N+FPCC_Z,%d1
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
	eori	#FPCC_NAN+FPCC_Z,%d1
	andi	#FPCC_NAN+FPCC_Z+FPCC_N,%d1
	rts

	.balign 32
	| c NGE = NAN + (N x !Z) = NAN + !(!N + Z)
	btst	#FPCC_NAN_BIT,%d1
	bne	1f
	eori	#FPCC_N,%d1
	andi	#FPCC_N+FPCC_Z,%d1
	rts

	.balign 32
	| d NGT = NAN + Z + N
	not	%d1
	andi	#FPCC_NAN+FPCC_Z+FPCC_N,%d1
	rts

	.balign 32
	| e NEQ = !Z
	andi	#FPCC_Z,%d1
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
	bvs	1f	| -1 => overflow (FIXME: is this the correct condition?)
	.endif
	| Dn.w != -1
	| add displacement to PC
	sub	#2,%d3
	lea.l	(%a5,%d3.w),%a5
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
	rts


constant_rom:
	.long 0x40000000,0xc90fdaa2,0x2168c235  | 00 pi
	.long 0x00000000,0x00000000,0x00000000  | 01
	.long 0x00000000,0x00000000,0x00000000  | 02
	.long 0x00000000,0x00000000,0x00000000  | 03
	.long 0x00000000,0x00000000,0x00000000  | 04
	.long 0x00000000,0x00000000,0x00000000  | 05
	.long 0x00000000,0x00000000,0x00000000  | 06
	.long 0x00000000,0x00000000,0x00000000  | 07
	.long 0x00000000,0x00000000,0x00000000  | 08
	.long 0x00000000,0x00000000,0x00000000  | 09
	.long 0x00000000,0x00000000,0x00000000  | 0a
	.long 0x3ffd0000,0x9a209a84,0xfbcff798  | 0b log10(2)
	.long 0x40000000,0xadf85458,0xa2bb4a9a  | 0c e
	.long 0x3fff0000,0xb8aa3b29,0x5c17f0bc  | 0d log2(e)
	.long 0x3ffd0000,0xde5bd8a9,0x37287195  | 0e log10(e)
	.long 0x00000000,0x00000000,0x00000000  | 0f 0.0
	.long 0x00000000,0x00000000,0x00000000  | 10
	.long 0x00000000,0x00000000,0x00000000  | 11
	.long 0x00000000,0x00000000,0x00000000  | 12
	.long 0x00000000,0x00000000,0x00000000  | 13
	.long 0x00000000,0x00000000,0x00000000  | 14
	.long 0x00000000,0x00000000,0x00000000  | 15
	.long 0x00000000,0x00000000,0x00000000  | 16
	.long 0x00000000,0x00000000,0x00000000  | 17
	.long 0x00000000,0x00000000,0x00000000  | 18
	.long 0x00000000,0x00000000,0x00000000  | 19
	.long 0x00000000,0x00000000,0x00000000  | 1a
	.long 0x00000000,0x00000000,0x00000000  | 1b
	.long 0x00000000,0x00000000,0x00000000  | 1c
	.long 0x00000000,0x00000000,0x00000000  | 1d
	.long 0x00000000,0x00000000,0x00000000  | 1e
	.long 0x00000000,0x00000000,0x00000000  | 1f
	.long 0x00000000,0x00000000,0x00000000  | 20
	.long 0x00000000,0x00000000,0x00000000  | 21
	.long 0x00000000,0x00000000,0x00000000  | 22
	.long 0x00000000,0x00000000,0x00000000  | 23
	.long 0x00000000,0x00000000,0x00000000  | 24
	.long 0x00000000,0x00000000,0x00000000  | 25
	.long 0x00000000,0x00000000,0x00000000  | 26
	.long 0x00000000,0x00000000,0x00000000  | 27
	.long 0x00000000,0x00000000,0x00000000  | 28
	.long 0x00000000,0x00000000,0x00000000  | 29
	.long 0x00000000,0x00000000,0x00000000  | 2a
	.long 0x00000000,0x00000000,0x00000000  | 2b
	.long 0x00000000,0x00000000,0x00000000  | 2c
	.long 0x00000000,0x00000000,0x00000000  | 2d
	.long 0x00000000,0x00000000,0x00000000  | 2e
	.long 0x00000000,0x00000000,0x00000000  | 2f
	.long 0x3ffe0000,0xb17217f7,0xd1cf79ac  | 30 ln(2)
	.long 0x40000000,0x935d8ddd,0xaaa8ac17  | 31 ln(10)
	.long 0x3fff0000,0x80000000,0x00000000  | 32 10^0
	.long 0x40020000,0xA0000000,0x00000000  | 33 10^1
	.long 0x40050000,0xC8000000,0x00000000  | 34 10^2
	.long 0x400C0000,0x9C400000,0x00000000  | 35 10^4
	.long 0x40190000,0xBEBC2000,0x00000000  | 36 10^8
	.long 0x40340000,0x8E1BC9BF,0x04000000  | 37 10^16
	.long 0x40690000,0x9DC5ADA8,0x2B70B59E  | 38 10^32
	.long 0x40D30000,0xC2781F49,0xFFCFA6D5  | 39 10^64
	.long 0x41A80000,0x93BA47C9,0x80E98CE0  | 3a 10^128
	.long 0x43510000,0xAA7EEBFB,0x9DF9DE8E  | 3b 10^256
	.long 0x46A30000,0xE319A0AE,0xA60E91C7  | 3c 10^512
	.long 0x4D480000,0xC9767586,0x81750C17  | 3d 10^1024
	.long 0x5A920000,0x9E8B3B5D,0xC53D5DE5  | 3e 10^2048
	.long 0x75250000,0xC4605202,0x8A20979B  | 3f 10^4096


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
