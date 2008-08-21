.global setjmp, longjmp

.section _st1

| struct jmp_buf
.equ jmp_buf.reg,0
.equ jmp_buf.sp,11*4
.equ jmp_buf.pc,12*4

/* int setjmp(jmp_buf env); */
setjmp:
	move.l	4(%sp),%a0		| get env
	
	move.l	%usp,%a1
	movem.l	%d3-%d7/%a1-%a7,(%a0)	| save %d3-%d7/%a2-%a7 and %usp
	move.l	(%sp),jmp_buf.pc(%a0)	| save return address
	
	clr	%d0
	rts

/* void longjmp(jmp_buf env, int val); */
longjmp:
	move.l	4(%sp),%a0		| get env
	
	move	8(%sp),%d0
	bne.s	0f
	move	#1,%d0			| cannot return 0 (reserved for setjmp)
	
0:	movem.l	(%a0)+,%d3-%d7/%a1-%a7	| restore %d3-%d7/%a2-%a7 and %usp
	move.l	%a1,%usp
	move.l	(%a0),(%sp)		| restore return address
	
	rts
