.section .text

.global __errno
| this is a temporary hack to get a pointer to our errno
| until we get a real loader linker in the kernel
__errno:
	trap	#1
	rts

.global cerror, caerror
/*
 * common routine to handle errors from system calls: set errno appropriately
 * and return -1.
 */
cerror:
	jbsr	__errno
	move	%d0,(%a0)
	moveq.l	#-1,%d0
	rts

caerror:
	jbsr	__errno
	move	%d0,(%a0)
	sub.l	%a0,%a0
	rts
