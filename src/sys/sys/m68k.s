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

G = 0x5c00
seconds = G+0
realtime = G+4
onkey = G+180 | see globals.h
powerstate = G+182 | see globals.h

| power off the CPU and LCD, and maintain the realtime clock.
| wake on int 6 (ON key), int 4 (link activity), and int 3 (1 Hz clock)
| void cpupoweroff();
.global cpupoweroff
cpupoweroff:
	move	%sr,%d0
	move	#0x2700,%sr
	move.l	realtime,%d2	| realtime.tv_sec
	sub.l	seconds,%d2
	move	%d0,%sr
	
	lea.l	0x60001c,%a0
	move.b	(%a0),%d1
	move.b	#0x3c,(%a0)	| shut off LCD
	
	clr	onkey
	move	#1,powerstate
	
0:	move.b	#0x0c,0x600005	| bit 3 => int 4, bit 2 => int 3
	tst	onkey
	beq	0b
	
	clr	powerstate
	
	move.b  %d1,(%a0)	| turn on LCD
	
	move	#0x2700,%sr
	add.l	seconds,%d2
	move.l	%d2,realtime
	move	%d0,%sr
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
