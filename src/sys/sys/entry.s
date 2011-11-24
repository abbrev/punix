/*
 * Punix
 * Copyright (C) 2003 PpHd
 * Copyright 2004, 2005, 2010, 2011 Chris Williams
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
.global TRAP_1
.global TRAP_2
.global TRAP_3
.global TRAP_4
.global TRAP_5
.global TRAP_6
.global TRAP_7
.global TRAP_8
.global TRAP_9
.global TRAP_10
.global TRAP_11
.global TRAP_12
.global TRAP_13
.global TRAP_14
.global TRAP_15
.global _WaitKeyboard
.global CheckBatt

.global _syscall

.section _st1,"rx"
.even

| XXX see h/globals.h
G = 0x5c00
seconds = G+0
onkey = G+180
powerstate = G+182

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

buserr:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#2,-(%sp)
	bra	exception

ADDRESS_ERROR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#3,-(%sp)
	bra	exception

ILLEGAL_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#4,-(%sp)
	bra	exception

ZERO_DIVIDE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#5,-(%sp)
	bra	exception

CHK_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#6,-(%sp)
	bra	exception

I_TRAPV:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#7,-(%sp)
	bra	exception

PRIVILEGE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#8,-(%sp)
	bra	exception

TRACE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#9,-(%sp)
	bra	exception

| Line 1010 emulator. Send SIGILL
| Are there valid 68010+ instructions starting with 1010?
LINE_1010:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#10,-(%sp)
	bra	exception

| Line 1111 emulator. Send SIGILL
| use this only if fpuemu is not included in the kernel
LINE_1111:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#11,-(%sp)
	bra	exception

SPURIOUS:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#24,-(%sp)
	bra	exception

Int_7:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#31,-(%sp)
	bra	exception

TRAP_1:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#33,-(%sp)
	bra	exception

TRAP_2:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#34,-(%sp)
	bra	exception

TRAP_3:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#35,-(%sp)
	bra	exception

TRAP_4:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#36,-(%sp)
	bra	exception

TRAP_5:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#37,-(%sp)
	bra	exception

TRAP_6:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#38,-(%sp)
	bra	exception

TRAP_7:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#39,-(%sp)
	bra	exception

TRAP_8:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#40,-(%sp)
	bra	exception

TRAP_9:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#41,-(%sp)
	bra	exception

TRAP_10:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#42,-(%sp)
	bra	exception

TRAP_11:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#43,-(%sp)
	bra	exception

TRAP_12:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#44,-(%sp)
	bra	exception

TRAP_13:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#45,-(%sp)
	bra	exception

TRAP_14:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#46,-(%sp)
	bra	exception

TRAP_15:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	move	#47,-(%sp)
	|bra	exception
	| fall through

| common point for all exceptions
exception:
	pea.l	5*4+2(%sp)
	jbsr	handle_exception
	addq.l	#6,%sp
	bra	trapret0	| we are returning to user mode (= int level 0)

| Scan for the hardware to know what keys are pressed 
Int_1:
	tst	powerstate
	bne	1f		| ZZZZzzz
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	
	move	5*4(%sp),-(%sp)		| old ps
	bsr	hardclock
	addq.l	#2,%sp

| common exit point for all traps
trapret:
	| check that we are returning to interrupt level 0 (no interrupt)
	move	5*4(%sp),%d0
	and	#0x0700,%d0
	bne.b	0f
	
trapret0:
	| we are returning to interrupt level 0
	move.l	%usp,%a0
	pea.l	(%a0)		| void *usp
	pea.l	(%sp)		| void **usp
	pea.l	7*4+2(%sp)	| void **pc
	move	8*4(%sp),-(%sp)	| unsigned short ps
	| void return_from_int(unsigned short ps, void **pc, void **usp);
	jbsr	return_from_int
	add.l	#2*4+2,%sp
	move.l	(%sp)+,%a0
	move.l	%a0,%usp
	
0:	movem.l	(%sp)+,%d0-%d2/%a0-%a1
1:	rte

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

Int_3:
.if 1
	addq.l	#1,seconds	| ++seconds
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
	bra	trapret

| System timers.
Int_5:
	tst	powerstate
	bne	1f		| ZZZZzzz
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	audiointr	| just call the C routine
	bra	trapret
1:	rte

| ON Int.
|	2ND / DIAMOND : Off
|	ESC : Reset
Int_6:
	move.b	#0,0x60001a	| acknowledge the key press
	move	#1,onkey
	rte

_WaitKeyboard:
	moveq	#0x58,%d0
	dbf	%d0,.
	rts

	|long dreg[5];  /* %d3-%d7 */
	|long *usp;     /* %usp */
	|long areg[5];  /* %a2-%a6 */
	|long *sp;      /* %a7 */
	|/* note: the following two align with a trap stack frame */
	|short sr;      /* %sr */
	|void *pc;      /* return address */

| Assembly interface for C version; prototype of C version:
| uint32_t syscall(unsigned callno, struct context *ctx)
_syscall:
	/* push a struct context onto the stack */
	move.l	%usp,%a1
	movem.l	%d3-%d7/%a1-%a7,-(%sp)
	
	pea.l	(%sp)		| struct context *ctx
	move	%d0,-(%sp)	| int callno
	bsr	syscall
	adda.l	#6,%sp
	
	movem.l	(%sp)+,%d3-%d7/%a1-%a7
	move.l	%a1,%usp

	|rte
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	jbra	trapret0

.global return_from_vfork
return_from_vfork:
	moveq	#0,%d0
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	jbra	trapret0
	rte
