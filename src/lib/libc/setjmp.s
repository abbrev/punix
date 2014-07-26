/*
 * This is a standard setjmp/longjmp that saves/restores all callee-saved
 * registers (%d3-%d7 and %a2-%a6). One improvement in longjmp (as found in
 * glibc) is to ensure that the saved stack pointer is not lower than the
 * current stack pointer. If it is lower, then it should not jump to the saved
 * jump point.
 */

.global setjmp, longjmp

.section _st1

| struct jmp_buf
.equ jmp_buf.reg,0   | %d3-%d7/%a2-%a6
.equ jmp_buf.sp,10*4 | %a7
.equ jmp_buf.pc,11*4 | return address

/* int setjmp(jmp_buf env); */
setjmp:
	move.l	4(%sp),%a0		| get env
	
	movem.l	%d3-%d7/%a2-%a7,(%a0)	| save %d3-%d7/%a2-%a7
	move.l	(%sp),jmp_buf.pc(%a0)	| save return address
	
	clr	%d0
	rts

/* void longjmp(jmp_buf env, int val); */
longjmp:
	move.l	4(%sp),%a0		| get env
	
	move	8(%sp),%d0
	bne	0f
	move	#1,%d0			| cannot return 0 (reserved for setjmp)
	
0:	movem.l	(%a0)+,%d3-%d7/%a2-%a7	| restore %d3-%d7/%a2-%a7
	move.l	(%a0),(%sp)		| restore return address
	
	rts
