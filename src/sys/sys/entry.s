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

	.long 0xbeef1001
buserr:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	bus_error
	bra	trapret

	.long 0xbeef1002
SPURIOUS:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	spurious
	bra	trapret

	.long 0xbeef1003
ADDRESS_ERROR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	address_error
	bra	trapret

	.long 0xbeef1004
ILLEGAL_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	illegal_instr
	bra	trapret

	.long 0xbeef1005
| send signal SIGFPE
ZERO_DIVIDE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	zero_divide
	bra	trapret

	.long 0xbeef1006
CHK_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	chk_instr
	bra	trapret

	.long 0xbeef1007
I_TRAPV:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	i_trapv
	bra	trapret

	.long 0xbeef1008
| send signal SIGILL
PRIVILEGE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	privilege
	bra	trapret

	.long 0xbeef1009
TRACE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	pea.l	5*4(%sp)
	bsr	trace
	bra	trapret

	.long 0xbeef100a
| I don't know (send signal SIGILL?)
| Are there valid 68010+ instructions starting with 1010?
LINE_1010:
	rte

| Scan for the hardware to know what keys are pressed 
Int_1:
	tst	powerstate
	bne	1f		| ZZZZzzz
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	
	move	5*4(%sp),-(%sp)		| old ps
	bsr	hardclock
	addq.l	#2,%sp
	bra	trapret2

trapret:
	addq.l	#4,%sp
trapret2:
	move	5*4(%sp),%d0
	and	#0x0700,%d0
	bne.b	0f
	
	move.l	%usp,%a0
	pea.l	(%a0)		| void *usp
	pea.l	(%sp)		| void **usp
	pea.l	7*4+2(%sp)	| void **pc
	move	8*4(%sp),-(%sp)	| unsigned short ps
	| void return_from_trap(unsigned short ps, void **pc, void **usp);
	jbsr	return_from_trap
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
	bra	trapret2

| System timers.
Int_5:
	tst	powerstate
	bne	1f		| ZZZZzzz
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	audiointr	| just call the C routine
	bra	trapret2
1:	rte

| ON Int.
|	2ND / DIAMOND : Off
|	ESC : Reset
Int_6:
	move.b	#0,0x60001a	| acknowledge the key press
	move	#1,onkey
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
	rte

.global return_from_vfork
return_from_vfork:
	moveq	#0,%d0
	rte

| unused for now
_trap:
	rte
