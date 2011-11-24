.include "syscalls.inc"

| XXX: we don't support _exit because TIGCC/GCC4TI treat it specially for
| some reason, which makes it resolve to 0x412000
.global _Exit
_Exit:
	move	#SYS__exit,%d0
	trap	#0
0:	nop
	bra	0b

