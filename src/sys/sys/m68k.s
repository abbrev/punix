.section _st1, "x"

| NB: cpuidle() works on both the TI-89 and TI-92+, but it doesn't power off
| the FlashROM on the 92+
| void cpuidle(void);
.global cpuidle
cpuidle:
	move.w	%sr,%d1
	move.w	#0x2700,%sr
	move.w	#0x280,0x600018
	moveq	#0x1f,%d0
	move.b	%d0,0x600005 | shut off the cpu until an interrupt is triggered
	nop
	move.w	#0x2000,%sr  | let interrupt handlers run
	move.w	%d1,%sr
	rts


| void delay(unsigned long n);
.global delay
delay:
	move.l	4(%sp),%d0
0:	subq.l	#1,%d0
	bne	0b
	rts

| int backtrace(void **buffer, int size);
.global backtrace
backtrace:
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
		
		move.l	4(%a1),%d2
		move.l	(%a1),%a1	| get the next frame
1:		dbra	%d0,0b
2:	sub.l	4(%sp),%a0
	move.l	%a0,%d0
	lsr.l	#2,%d0
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
