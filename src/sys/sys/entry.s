/*
 * Punix
 * Copyright (C) 2003 PpHd
 * Copyright 2004, 2005 Chris Williams
 * 
 * $Id$
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

.global buserr
.global SPURIOUS
.global ADDRESS_ERROR
.global ILLEGAL_INSTR
.global ZERO_DIVIDE
.global CHK_INSTR
.global I_TRAPV
.global PRIVILEGE
.global TRACE
.global LINE_1010
.global LINE_1111

.global Int_1
.global Int_2
.global Int_3
.global Int_4
.global Int_5
.global Int_6
.global Int_7
.global _WaitKeyboard
.global CheckBatt

.global _syscall
.global _trap
.global setup_env

|.include "procglo.inc"
.include "signal.inc"

.section _st1,"rx"
.even

/*
 * Bus or Address error exception stack frame:
 *        15     5    4     3    2            0 
 *       +---------+-----+-----+---------------+
 * sp -> |         | R/W | I/N | Function code |
 *       +---------+-----+-----+---------------+
 *       |                 High                |
 *       +- - Access Address  - - - - - - - - -+
 *       |                 Low                 |
 *       +-------------------------------------+
 *       |        Instruction register         |
 *       +-------------------------------------+
 *       |          Status register            |
 *       +-------------------------------------+
 *       |                 High                |
 *       +- - Program Counter - - - - - - - - -+
 *       |                 Low                 |
 *       +-------------------------------------+
 *       R/W (Read/Write): Write = 0, Read = 1.
 *       I/N (Instruction/Not): Instruction = 0, Not = 1.
 */

	.long 0xbeef1001
buserr:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	bus_error
	bra.s	trapret

	.long 0xbeef1002
SPURIOUS:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	spurious
	bra.s	trapret

	.long 0xbeef1003
ADDRESS_ERROR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	address_error
	bra.s	trapret

	.long 0xbeef1004
ILLEGAL_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move.l	5*4+2(%sp),-(%sp)
	bsr	illegal_instr
	addq.l	#4,%sp
	bra.s	trapret

	.long 0xbeef1005
| send signal SIGFPE
ZERO_DIVIDE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move.l	5*4+2(%sp),%a0
	
	| get the address of the divs/divu instruction
	tst	-(%a0)
	bne	0f
	subq.l	#2,%a0	| immediate 16-bit operand
0:	pea.l	(%a0)
	
	bsr	zero_divide
	addq.l	#4,%sp
	bra.s	trapret

	.long 0xbeef1006
CHK_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move.l	5*4+2(%sp),-(%sp)
	bsr	chk_instr
	addq.l	#4,%sp
	bra.s	trapret

	.long 0xbeef1007
I_TRAPV:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move.l	5*4+2(%sp),-(%sp)
	bsr	i_trapv
	addq.l	#4,%sp
	bra.s	trapret

	.long 0xbeef1008
| send signal SIGILL
PRIVILEGE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move.l	5*4+2(%sp),-(%sp)
	bsr	privilege
	addq.l	#4,%sp
	bra.s	trapret

	.long 0xbeef1009
TRACE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move.l	5*4+2(%sp),-(%sp)
	bsr	trace
	addq.l	#4,%sp
	bra.s	trapret

	.long 0xbeef100a
| I don't know (send signal SIGILL?)
| Are there valid 68010+ instructions starting with 1010?
LINE_1010:
	rte

| Scan for the hardware to know what keys are pressed 
Int_1:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	
	move	5*4(%sp),-(%sp)		| old ps
	bsr	hardclock
	addq.l	#2,%sp

trapret:
	move    5*4(%sp),%d0
	and     #0x2000,%d0
	bne.b   0f
	
	move.l  %usp,%a0
	pea.l   (%a0)                   | void *usp
	pea.l   (%sp)                   | void **usp
	move.l  7*4+2(%sp),-(%sp)       | void *pc
	| call signal handler here (prototype is dosig(void *pc, void **usp))
	addq.l  #2*4,%sp
	move.l  (%sp)+,%a0
	move.l  %a0,%usp
	
0:	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

/*
 * Does nothing. Why ? Look:
 *  Triggered when the *first* unmasked key (see 0x600019) is *pressed*.
 *  Keeping the key pressed, or pressing another without releasing the first
 *  key, will not generate additional interrupts.  The keyboard is not
 *  debounced in hardware and the interrupt can occasionally be triggered many
 *  times when the key is pressed and sometimes even when the key is released!
 *  So, you understand why you don't use it ;)
 *  Write any value to 0x60001B to acknowledge this interrupt.
 */
Int_2:	move.w	#0x2600,%sr
	move.w	#0x00FF,0x60001A	| acknowledge Int2
oldInt_3:	rte				| Clock for int 3 ?

G = 0x5c00

Int_3:
.if 0
	addq.l	#1,G+0	| walltime.tv_sec++
	|clr.l	G+4	| walltime.tv_nsec = 0
	move.l	#-1000000000/256,G+4	| walltime.tv_nsec = -TICK
.else
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	jbsr	updwalltime
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
.endif
	rte

| Link Auto-Int
Int_4:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	linkintr	| just call the C routine
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

| System timers.
Int_5:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	audiointr	| just call the C routine
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

| ON Int.
|	2ND / DIAMOND : Off
|	ESC : Reset
Int_6:
	/* FIXME: handle ON key */
	rte

| send signal SIGSEGV
Int_7:
	/* FIXME */
	jmp	the_beginning

_WaitKeyboard:
	moveq	#0x58,%d0
	dbf	%d0,.
	rts

	.long	0xdeadd00d
| Assembly interface for C version; prototype of C version:
| uint32_t syscall(unsigned callno, void **usp, struct syscallframe *sfp)
_syscall:
	movem.l	%d3-%d7/%a2-%a6,-(%sp)	| XXX: this is needed for vfork!
	
	pea.l	(%sp)		| struct syscallframe *sfp
	
	move.l	%usp,%a0
	move.l	%a0,-(%sp)	| void **usp
	
	move	%d0,-(%sp)	| int callno
	
	bsr	syscall
	adda.l	#10,%sp
	
	movem.l	(%sp)+,%d3-%d7/%a2-%a6
	rte

| jmp_buf
.equ jmp_buf.reg,0
.equ jmp_buf.sp,10*4
.equ jmp_buf.usp,11*4
.equ jmp_buf.retaddr,12*4

| struct syscallframe
.equ syscallframe.regs,0
.equ syscallframe.sr,10*4
.equ syscallframe.pc,10*4+2

/* void setup_env(jmp_buf env, struct syscallframe *sfp, long *sp);
 * Setup the execution environment "env" using the syscallframe "sfp", the stack
 * pointer "sp", and the current user stack pointer. The syscallframe is pushed
 * onto the stack (*sp), and the new stack pointer is saved in the environment.
 * FIXME: make this routine work with the newer struct syscallframe
 * FIXME: make this routine cleaner and more elegant!
 */
setup_env:
	move.l	12(%sp),%a0		| %a0 = sp
	move.l	8(%sp),%a1		| %a1 = sfp
	
	| setup the new trap frame
	move.l	syscallframe.pc(%a1),-(%a0)	| push pc
	clr	-(%a0)				| clear sr
	
	move.l	4(%sp),%a1		| %a1 = env
	move.l	%a0,jmp_buf.sp(%a1)	| sp
	
	move.l	%usp,%a0
	move.l	%a0,jmp_buf.usp(%a1)	| usp
	
	move.l	1f(%pc),jmp_buf.retaddr(%a1)
	
	move.l	8(%sp),%a0		| %a0 = tfp
	move	#10-1,%d0
0:	move.l	-(%a0),(%a1)+		| copy the saved regs to the child's env
	dbra	%d0,0b
	
	rts

/* the child process resumes here  */
1:
	move	#0,%d0	| return 0 to the child process
	rte

| unused for now
_trap:
	rte
