/*
 * Punix, Operating System for TI-89/TI-92+/V200
 * Copyright 2004 Chris Williams
 * Some parts from PedroM are Copyright (C) 2003 PpHd
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

|.include "vectorsglo.inc"
|.include "glo.inc"
|.include "cellglo.inc"
.include "vectors.inc"

.section _st1,"x"
.global the_beginning
|***************************************************************
| Init the Hardware
|***************************************************************

the_beginning:
	| Init the calc
	bclr.b	#1,0x70001d		| disable screen on HW2 (must be the first instruction)
	move.w	#0x2700,%sr		| Prevent Auto Ints
	lea	SSP_INIT,%sp		| Setup Supervisor Stack 
	| XXX: isn't the %sp initialized from the vectors table??
	
	moveq	#0,%d0
	
	| Setup IO ports
	lea	0x600000,%a5		| Address of Port IO 6
	lea	0x700000,%a6		| Address of Port IO 7
	
	move.w	#0x40,0x10(%a6)		| Link Speed on HW2

	move.b	%d0,0x15(%a5)	| OSC2 and LCD mem stopped on HW1 (do not clr.b since it reads before set)

	move.w	#0xFFFF,0x18(%a5)	| Setup Batt Level to the lowest trigger level (FIXME: Works on HW2 ?)
	move.w	#0x8000,(%a5)		| Pin 100 Enable / Write Protect Vectors disable / 256K of RAM
	move.w	#0x374,%d1		| make sure the hardware has stabilized
	dbra	%d1,.
.ifndef	TI89
	btst.b	#0x2,(%a5)		| Batts are below the lowest level?
	beq.s	0f			| FIXME: bne ?
.endif
		move.b	%d0,(%a5)		| Do not set Pin100 
0:

	| Unprotect access to special IO ports
	lea	0x1C5EA4,%a0
	nop
	nop
	nop
	move.w	#0x2700,%sr
	move.w	%d0,(%a0)
	nop
	nop
	nop
	move.w	#0x2700,%sr
	move.w	%d0,(%a0)
	
	| Can not registers to access IO ports <= we have disabled the hardware protection, so a hack may be used. (using registers seems to work fine -- caw)
	| Set Protected IO ports
	ori.b	#4+2+1,0x70001F		| HW2: ???/Enable OSC2/5 contrasts bits

	moveq	#0,%d0			| %d0.l = 0 (Even if %d0=0, I must set it again <= AntiHack).
	move.w	#0x003F,0x700012	| Allow Execution of all Flash ROM on HW2
	move.w	%d0,0x45E00		| Allow Execution of all Flash ROM on HW1
	move.w	%d0,0x85E00
	move.w	%d0,0xC5E00
	
	| Allow Execution of all RAM on HW2
| clr.l 0x700000 is potentially dangerous since we read it before setting it !
	move.l	%d0,0x700000
	move.l	%d0,0x700004
	
	| Protect access to special IO ports
	nop
	nop
	nop
	move.w	#0x2700,%sr
	move.w	(%a0),%d0
	
	| Setup IO ports
	moveq.l	#0,%d0
.ifdef	TI89
		move.b	#0xFF,0x3(%a5)	| Setup Bus Wait State (Very fast).
.else
		move.b	%d0,0x3(%a5)	| Setup Bus Wait States (Very slow).
.endif
	move.b	%d0,0xC(%a5)		| Setup Link Port (Nothing enable link ports)
	move.w	#0x3180,0x12(%a5)	| Set LCD logical width / height
	move.w	#0x4C00/8,0x10(%a5)	| Set LCD memory address on HW1
	move.b	%d0,0x17(%a6)		| Set LCD memory address on HW2.
	move.b	#0x21,0x1C(%a5)		| Set LCD RS
	move.b	#0xB2,0x17(%a5)		| Reset 0x600017 cycles on HW1.
	move.b	#0x1B,0x15(%a5)		| Enable Timer Interrupts / Increment rate of 0x600017 (OSC2/2^9) / Enable 0x600017 / Disable Int 3 / Enable OSC2 / Enable LCD on HW1
	bset.b	#1,0x1D(%a6)		| Enable LCD on HW2
	move.w	#0xFFFF,0x1A(%a5)	| acknowledge AutoInt 6 & AutoInt 2
	
	| Clear 256K of RAM
	suba.l	%a0,%a0
	moveq	#-1,%d0			| 256K / 4 = 64K-1 = 0xFFFF
0:	clr.l	(%a0)+
	dbra	%d0,0b
		
	| Copy Vector Table
	bsr	InstallVectors

.if 0
	| Check the range of the system
	| (Use only on Emulator)
	lea	OSTooBig_str(%pc),%a0		
	move.l	#BASE_END,%a1			| Check if the system 
	cmp.l	#START_ARCHIVE,%a1		| Does not overflow
	bhi	BOOT_ERROR
	move.l	#BASE1_END,%a1
	cmp.l	#ROM_BASE+0x18000,%a1
	bhi	BOOT_ERROR
.endif
	
	move.w	#0x4C00/8,0x600010		| Set 4C00 as VRAM for HW1
	clr.b	0x700017			| Set 4C00 as VRAM for HW2
	

|***************************************************************
| Init the Operating System
|***************************************************************

/*
 * Initialise global variables
 */
	| XXX clear screen
	movea	#0x4c00,%a0
	move	#30*128/4-1,%d0
	moveq.l	#0,%d1
0:	move.l	%d1,(%a0)+
	dbra	%d0,0b
	
|.include "proc.inc"

	bsr	kmain
	
	| set up a trap frame and "return" to icode
	lea	-512(%sp),%a0	| XXX constant
	move.l	%a0,%usp	| set initial stack for icode
	|tst	-(%sp)		| need this for M68010+
	pea	icode		| return address
	clr	-(%sp)		| return %sr (user mode)
	rte			| "return" to icode
	/* NB: this works only on M68000. To work on M68010 and up, add another
	 * word on the stack above the return address */
	 
SYS_exit	= 1	/* put these in a header please */
SYS_write	= 4
SYS_execve	= 59
.macro	sys call
	move	#SYS_\call,%d0
	trap	#0
.endm

.even
	.long	0xbeef0001
	
	.global _exit
_exit:
	sys	exit
0:	bra.s	0b

	.global write
write:
	sys	write
	bcs	cerror
	rts

	.global execve
execve:
	sys	execve
	bcs	cerror
	rts

/*
 * common routine to handle errors from system calls: set errno appropriately
 * and return -1.
 */
cerror:
	| move	%d0,errno
	moveq.l	#-1,%d0
	rts

/*
 * icode is the first code to run in userland. It execve()'s the init program
 * stored at "/etc/init". If that fails, it prints a message and _exit()'s.
 */
icode:
	pea	_env(%pc)	| env ("TERM=punix")
	pea	_argv(%pc)	| argv ("/etc/init", "-00000")
	pea	_argv0(%pc)	| path ("/etc/init")
	bsr	execve
	
	move	#-1,-(%sp)	| status
	bsr	_exit

	pea	1f(%pc)
	jbsr	panic
	
	bra	.
	
1:	.asciz	"execve and _exit failed"

| strings
_argv0:	.asciz	"/etc/init"
_argv1:	.asciz	"-00000"	| what is this exactly?
_env0:	.asciz	"HOME=/"
_env1:	.asciz	"TERM=punix"

| vectors
	.even
_argv:	.long	_argv0		| "init"
	.long	_argv1
	.long	0		| NULL
_env:	.long	_env0
	.long	_env1
	.long	0		| NULL

.even
.global userstart
userstart:
	move	(%sp),%d0		| argc
	lea	2(%sp),%a0		| argv
	move	%d0,%d1
	mulu	#4,%d1
	lea	6(%sp,%d1.w),%a1	| env
	
	move.l	%a1,-(%sp)	| char **env
	move.l	%a0,-(%sp)	| char **argv
	move	%d0,-(%sp)	| int argc
	bsr	usermain	| usermain(argc, argv, env)
	
	move	%d0,-(%sp)
	bsr	_exit
	bra	.

.even
InstallVectors:
	bclr.b	#2,0x600001		| unprotect vector table
	| Copy org Vectors
	lea	vectors_table(%pc),%a0
	lea	0x40000,%a1			| GHOST SPACE (To avoid unprotection)
	moveq	#0x3F,%d0
0:
	move.l	(%a0)+,(%a1)+
	dbra	%d0,0b
	bset.b	#2,0x600001		| protect vector table
	rts
