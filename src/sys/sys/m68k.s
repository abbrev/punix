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
	bne.s	0b
	rts
