.section .text, "x"

| void cpuidle(int mask);
.global cpuidle
cpuidle:
	move.w	%sr,%d1
	move.w	#0x2700,%sr
	move.w	#0x280,0x600018 | XXX: why do we do this?
	move	4(%sp),%d2	| interrupt mask
.ifdef TI89
	move.b	%d2,0x600005 | shut off the cpu until an interrupt is triggered
	nop
.else
	move	#(disable_flash_end-disable_flash)/2-1,%d0
	lea	disable_flash,%a0
	jbsr	exec_in_ram
.endif
	move.w	#0x2000,%sr  | let interrupt handlers run
	move.w	%d1,%sr
	rts

.ifndef TI89
| this must be run from RAM
disable_flash:
	move.w	0x185e00,%d0 | shut down flash rom
	move.b	%d2,0x600005 | shut off the cpu until an interrupt is triggered
	nop
	move.w	%d0,0x185e00
	nop
	nop
	rts
disable_flash_end:
.endif

exec_ram = 0x5c00+4+8 | XXX see globals.h

| input:
|  %a0 = code to execute in ram
|  %d0 = one less than the size of code, in 16-bit words, to execute in ram
.global exec_in_ram
exec_in_ram:
	lea     exec_ram,%a1
0:	move.w  (%a0)+,(%a1)+
	dbra    %d0,0b

	jbra     exec_ram        | Execute code in RAM

| void delay(unsigned long n);
.global delay
delay:
	move.l	4(%sp),%d0
0:	subq.l	#1,%d0
	bne	0b
	rts

| stack layout:
| arguments              ^
| return address         |
| saved frame pointer <+-+
| local variables      |
| arguments            |
| return address       |
| saved frame pointer -+<+
| local variables        |
|                        |
| frame pointer (%fp)  --+
| int backtrace(void **buffer, int size);
.global backtrace
backtrace:
	link	%fp,#0
	move.l	8(%sp),%a0	| buffer
	move	12(%sp),%d0	| size
	
	move.l	%fp,%a1
	|move.l	%fp,%d2
	bra	1f
	
0:
		move.l	%a1,%d1
		beq	2f	| it's zero
		btst	#0,%d1
		bne	2f	| not a valid pointer
		cmp.l	#0x40000,%d1	| upper limit of RAM
		bhi	2f
.if 0
		cmp.l	%d2,%d1
		blo	2f	| we went in the opposite direction
		move.l	%a1,%d2		| save this for the next round
.endif
		move.l	4(%a1),(%a0)+	| put return address in the buffer
		
		move.l	(%a1),%a1	| get the next frame
1:		dbra	%d0,0b
2:	move.l	%a0,%d0
	sub.l	8(%sp),%d0
	lsr.l	#2,%d0
	unlk	%fp
	rts

| int stacktrace(void **buffer, int size);
.global stacktrace
stacktrace:
	move.l	4(%sp),%a0
	move	8(%sp),%d0
	
	move.l	%fp,%a1
	move.l	%fp,%d2
	bra	1f
	
0:
		move.l	%a1,%d1
		beq	2f	| it's zero
		btst	#0,%d1
		bne	2f	| not a valid pointer
		cmp.l	#0x40000,%d1	| upper limit of RAM
		bhi	2f
		|cmp.l	%d2,%d1
		|blo	2f	| we went in the opposite direction
		move.l	%d1,(%a0)+	| put it in the buffer
		
		move.l	%a1,%d2
		move.l	(%a1),%a1	| get the next frame
1:		dbra	%d0,0b
2:	sub.l	4(%sp),%a0
	move.l	%a0,%d0
	lsr.l	#2,%d0
	rts

| int trygetlock(char *lockp);
| try to get a lock
| return non-zero if we have a lock, 0 if we don't
.global trygetlock
trygetlock:
	move.l	4(%sp),%a0
	tas	(%a0)	| try to get the lock
	seq	%d0	| set %d0=0xff if we got it, 0x00 if not
	ext.w	%d0	| extend to word
	rts

| void unlock(char *lockp);
.global unlock
unlock:
	move.l	4(%sp),%a0
	move.b	#0,(%a0)
	rts

| masklock:
| 2 byte atomic lock

| void initmask(masklock *lockp);
.global initmask
initmask:
	move.l	4(%sp),%a0
	clr.w	(%a0)
	rts

	.long	0xb00b1e5
| masklock mask(masklock *lockp);
| increment *lockp atomically and return the old value
.global mask
mask:
	move.l	4(%sp),%a0
	move	%sr,%d1
	move	#0x2700,%sr
	move.w	(%a0),%d0
	addq.w	#1,(%a0)
	move	%d1,%sr
	rts

/*
| note: these are defined as macros in ../h/punix.h

| void unmask(masklock *lockp);
| decrement *lockp atomically
.global unmask
unmask:
	move.l	4(%sp),%a0
	subq.w	#1,(%a0)
	rts

| void setmask(masklock *lockp, masklock value);
| set *lockp to value atomically
.global setmask
setmask:
	move.l	4(%sp),%a0
	move.w	8(%sp),(%a0)
	rts
*/

.include "lcd.inc"

.global copythird
| void copythird(void *plane, int offset)
copythird:
	move.l	(4,%sp),%a0	| plane
	move	(8,%sp),%d0	| offset
	move.l	#LCD_MEM,%a1
	add	%d0,%a0
	add	%d0,%a1
	movem.l	%d2-%d7/%a2-%a6,-(%sp)
	move	%sr,%d0
	move	%d0,-(%sp)
	|move	#0x2700,%sr

	movem.l	(%a0)+,%d0-%d7/%a2-%a6	| move 52 bytes
	movem.l	%d0-%d7/%a2-%a6,(%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(52,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(104,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(156,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(208,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(260,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(312,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(364,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(416,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(468,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(520,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(572,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(624,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(676,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(728,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(780,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(832,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(884,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(936,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(988,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(1040,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(1092,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(1144,%a1)
	movem.l	(%a0)+,%d0-%d7/%a2-%a6
	movem.l	%d0-%d7/%a2-%a6,(1196,%a1)
	movem.l	(%a0)+,%d0-%d7
	movem.l	%d0-%d7,(1248,%a1)

	move	(%sp)+,%d0
	move	%d0,%sr
	movem.l	(%sp)+,%d2-%d7/%a2-%a6
	rts

