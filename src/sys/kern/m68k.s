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
defspl	3 | time
defspl	4 | link port
defspl	5 | audio (link port)
|defspl	6 | on key
defspl	7 | block everything (except memory write error)

| void stop();
.global stop
stop:
	stop	#0x2700
	rts
