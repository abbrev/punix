.section .text

.global exit
exit:
	| do atexit callbacks here
	jbra	_Exit
