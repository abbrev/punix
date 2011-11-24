.section .text

.global cerror, caerror
/*
 * common routine to handle errors from system calls: set errno appropriately
 * and return -1.
 */
cerror:
	move	%d0,-(%sp)
	jbsr	seterrno
	lea.l	2(%sp),%sp
	moveq.l	#-1,%d0
	rts

caerror:
	move	%d0,-(%sp)
	jbsr	seterrno
	lea.l	2(%sp),%sp
	sub.l	%a0,%a0
	rts
