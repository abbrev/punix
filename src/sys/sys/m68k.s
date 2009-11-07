.section _st1, "x"

/* set the interrupt level to 'x' and return the old one */
.global splx
| void splx(int x);
splx:
	move	%sr,%d0
	move	4(%sp),%sr
	rts

| int spl'n'();
.macro	defspl	level
.global spl\level
spl\level:
	move	%sr,%d0 | save the old %sr
	move	#0x2000+(\level<<8),%sr
	rts
.endm

| XXX: not all of these levels are used
defspl	0 | 
defspl	1 | 256Hz timer
|defspl	2 | key press (never used)
.global splclock
splclock:
defspl	3 | time
defspl	4 | link port
defspl	5 | audio (link port)
|defspl	6 | on key
defspl	7 | block everything (except memory write error)

.global nop
nop:	rts

| void halt(void);
.global halt
halt:
	stop	#0x2700
	rts

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
	bne.s	0b
	rts
