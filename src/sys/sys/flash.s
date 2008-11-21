|
| flash.s, routines for writing and erasing Flash ROM
| Copyright (C) 2003, 2005 Patrick Pelissier
| Modified by Christopher Williams for Punix
|
| This program is free software ; you can redistribute it and/or modify it under
| the terms of the GNU General Public License as published by the Free Software
| Foundation; either version 2 of the License, or (at your option) any later
| version. 
| 
| This program is distributed in the hope that it will be useful, but WITHOUT
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
| FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
| 
| You should have received a copy of the GNU General Public License along with
| this program; if not, write to the  Free Software Foundation, Inc., 59 Temple
| Place, Suite 330, Boston, MA 02111-1307 USA 

.global FlashWrite, FlashErase |, archive, archive_end

archive     = (__ld_program_size + 0x200000 + 0xffff) |& 0xffff | !!!
archive_end = 0x400000

.section _st1,"x"
exec_ram = 0x5c00 | XXX

| short FlashWrite(const void *src asm("%a2"), void *dest asm("%a3"), size_t size asm("%d3"))
FlashWrite:
	movem.l	%d1-%d7/%a0-%a6,-(%sp)	| Save Registers
	move.w	%d0,-(%sp)		| Save %sr
	
	clr.w	%d5		| Set Error 
	
	| Check Batt
	bsr	BatTooLowFlash
	tst.b	%d0
	bne	FlashWriteProtect
	
	| Unprotect Flash (%sr = 0x2700)
	move.w	0x5EA4,%d0	| I read this because on Vti, the address 0x1C5EA4 & 0x5EA4 are the same, so if I write 0, it corrupts the heap. Very annoying, no ?
	lea	(0x1C5EA4).l,%a0
	bclr	#1,(0x600015)	|turn off data to the LCD (RAM is not read)
	nop
	nop
	nop
	move	#0x2700,%sr
	move.w	%d0,(%a0)
	nop
	nop
	nop
	move	#0x2700,%sr
	move.w	%d0,(%a0)
	bset	#1,(0x600015)	|turn on LCD
	
	| We don't check the stack, but we don't use it
	| Check if %a2 is in RAM
	cmp.l	#0x3FFFF,%a2
	bhi.s	FlashWriteProtect
	move.l	%a2,%d0
	andi.w	#1,%d0
	bne.s	FlashWriteProtect		| Not aligned
	| Check if %a3 is in Archive Memory
	cmp.l	#archive_end-1,%a3
	bhi.s	FlashWriteProtect
	cmp.l	#archive-1,%a3
	bls.s	FlashWriteProtect
	move.l	%a3,%d0
	andi.w	#1,%d0
	bne.s	FlashWriteProtect		| Not aligned
	| Check if %a3+%d3 is in the same block of memory
	addq.l	#1,%d3		| Word alignement (Long +1
	andi.w	#0xFFFE,%d3	| Clear low bit (does word instead of long)
	lea	-1(%a3,%d3.l),%a4	| -1 because the last byte we'll write is %a3+%d3-1
	move.l	%a3,%d0
	move.l	%a4,%d1
	swap	%d0
	swap	%d1
	cmp.w	%d0,%d1
	bne.s	FlashWriteProtect
	| Check if %a2+%d3 is in RAM
	lea	-1(%a2,%d3.l),%a4
	cmp.l	#0x3FFFF,%a4
	bhi.s	FlashWriteProtect
	
	lsr.l	#1,%d3		| Convert Byte to Word
	
	| Copy code to RAM and execute it
	move.w	#((FlashWrite_ExecuteInRam_End-FlashWrite_ExecuteInRam)/2-1),%d0
	lea	FlashWrite_ExecuteInRam(%pc),%a0
	lea	exec_ram,%a1
0:	move.w	(%a0)+,(%a1)+
	dbra	%d0,0b
	
	jsr	exec_ram	| Execute code in RAM
FlashWrite_Return:
	moveq	#1,%d5		| Of it is done

FlashWriteProtect:
	| Protect Flash
	lea	(0x1C5E00),%a0
	bclr	#1,(0x600015)	|turn off data to the LCD (RAM is not read)
	nop
	nop
	nop
	move	#0x2700,%sr
	move.w	(%a0),%d0
	nop
	nop
	nop
	move.w	#0x2700,%sr
	move.w	(%a0),%d0
	bset	#1,0x600015
	
	move.w	%d5,%d0			| Error code
	move.w	(%sp)+,%sr		| Return to User mode
	movem.l	(%sp)+,%d1-%d7/%a0-%a6	| Pop registers
	rts

| In :
|	%a2 -> Src
|	%a3 -> Dest
|	%d3 = Len in words 
FlashWrite_ExecuteInRam:
	subq.w	#1,%d3			| Because of Dbf
	blt.s	9f
	move.l	%a3,%a4			| A4 = Command register
	move.w	#0x5050,(%a4)		| Clear Statut Register
0:		move.w	(%a2)+,%d7	| Read value to write
		move.w	#0x1010,(%a3)	| Write Setup -- CHANGE HERE %a4 to %a3
		move.w	%d7,(%a3)+	| Write word
1:			move.w	(%a4),%d0	| Check it
			btst	#7,%d0
			beq.s	1b	| and wait that's done
		dbra	%d3,0b
	move.w	#0x5050,(%a4)
	move.w	#0xFFFF,(%a4)	| Read Memory
9:	rts
FlashWrite_ExecuteInRam_End:



| Low Level functions for Flash access (It doesn't use Trap #B for safe reason)
| short FlashErase(const void *dest asm("%a2"))
FlashErase:
	movem.l	%d1-%d7/%a0-%a6,-(%sp)	| Save Registers
	move.w	%d0,-(%sp)		| Save %sr
	
	clr.w	%d5		| Set Error 
	
	| Check Batt
	bsr	BatTooLowFlash
	tst.b	%d0
	bne.s	FlashEraseProtect
	
	| Unprotect Flash (%sr = 0x2700)
	move.w	0x5EA4,%d0	| I read this because on Vti, the address 0x1C5EA4 & 0x5EA4 are the same, so if I write 0, it corrupts the heap. Very annoying, no ?
	lea	(0x1C5EA4).l,%a0
	bclr	#1,(0x600015)	|turn off data to the LCD (RAM is not read)
	nop
	nop
	nop
	move	#0x2700,%sr
	move.w	%d0,(%a0)
	nop
	nop
	nop
	move	#0x2700,%sr
	move.w	%d0,(%a0)
	bset	#1,(0x600015)	|turn on LCD
	
	| We don't check the stack, but we don't use it
	| Check if %a2 is in Archive Memory
	cmp.l	#archive_end-1,%a2
	bhi.s	FlashEraseProtect
	cmp.l	#archive-1,%a2
	bls.s	FlashEraseProtect
	
	| Round to the upper 64K
	move.l	%a2,%d0
	andi.l	#0xFFFF0000,%d0
	move.l	%d0,%a2
	
	| Copy code to RAM and execute it
	move.w	#((FlashErase_ExecuteInRam_End-FlashErase_ExecuteInRam)/2-1),%d0
	lea	FlashErase_ExecuteInRam(%pc),%a0
	lea	exec_ram,%a1
0:	move.w	(%a0)+,(%a1)+
	dbra	%d0,0b
	jsr	exec_ram	| Execute code in RAM
FlashErase_Return:
	
	moveq	#1,%d5		| Of it is done
FlashEraseProtect:
	| Protect Flash
	lea	(0x1C5E00),%a0
	bclr	#1,(0x600015)	|turn off data to the LCD (RAM is not read)
	nop
	nop
	nop
	move	#0x2700,%sr
	move.w	(%a0),%d0
	nop
	nop
	nop
	move.w	#0x2700,%sr
	move.w	(%a0),%d0
	bset	#1,0x600015
	
	move.w	%d5,%d0			| Error code
	move.w	(%sp)+,%sr		| Return to User mode
	movem.l	(%sp)+,%d1-%d7/%a0-%a6	| Pop Registers
	rts

| In :
|	%a2 -> Src
FlashErase_ExecuteInRam:
	move.w	#0xFFFF,(%a2)	| Read ?
	move.w	#0x5050,(%a2)	| Set Statut register
	move.w	#0x2020,(%a2)	| Erase Setup
	move.w	#0xD0D0,(%a2)	| Erase Conform
0:		move.w	(%a2),%d0
		btst	#7,%d0
		beq.s	0b
	move.w	#0x5050,(%a2)
	move.w	#0xFFFF,(%a2)	| Read Memory
	rts
FlashErase_ExecuteInRam_End:

| In:
|	Nothing:
| Out:
|	%d0 = $FF if Batt are too low for flash
| Destroy:
|	%d0
BatTooLowFlash:
	jbsr	batt_check
	cmp.b	#2,%d0
	slt	%d0
	rts
