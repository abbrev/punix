.section .text

SYS__exit         = 1	/* put these in a header please */
SYS_read         = 3
SYS_write        = 4
SYS_open         = 5
SYS_close        = 6
SYS_getpid       = 20
SYS_uname        = 23
SYS_getuid       = 24
SYS_getppid      = 27
SYS_sysctl       = 30
SYS_sigaction    = 31
SYS_nice         = 34
SYS_kill         = 37
SYS_kmalloc      = 38
SYS_krealloc     = 39
SYS_kfree        = 40
SYS_dup          = 41
SYS_getgid       = 47
SYS_ioctl        = 54
SYS_execve       = 59
SYS_pause        = 63
SYS_vfork        = 66
SYS_getgroups    = 79
SYS_getpgrp      = 81
SYS_setitimer    = 83
SYS_wait         = 87
SYS_waitpid      = 88
SYS_getpriority  = 96
SYS_gettimeofday = 116
SYS_getrusage    = 117
SYS_settimeofday = 122
SYS_adjtime      = 140
SYS_getloadavg1  = 160
SYS_poweroff = 69

.macro	sys call
	move	#SYS_\call,%d0
	trap	#0
.endm

.macro	mksyscall name
	.global \name
\name:
	sys	\name
	bcs	cerror
	rts
.endm

.macro	mksyscalla name
	.global \name
\name:
	sys	\name
	bcs	caerror
	move.l	%d0,%a0
	rts
.endm
