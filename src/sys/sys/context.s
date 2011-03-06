.global csave, crestore

.section _st1

| struct context
.equ context.dreg,0*4
.equ context.usp, 5*4
.equ context.areg,6*4
.equ context.sp, 11*4
.equ context.pc, 12*4

/* int csave(struct context *ctx); */
csave:
	move.l	4(%sp),%a0		| get ctx
	
	move.l	%usp,%a1
	movem.l	%d3-%d7/%a1-%a7,(%a0)	| save %d3-%d7/%a2-%a7 and %usp
	move.l	(%sp),context.pc(%a0)	| save return address
	
	moveq	#0,%d0
	rts

/* void crestore(struct context *ctx); */
crestore:
	move.l	4(%sp),%a0		| get ctx
	
	movem.l	(%a0)+,%d3-%d7/%a1-%a7	| restore %d3-%d7/%a2-%a7 and %usp
	move.l	%a1,%usp
	move.l	(%a0),(%sp)		| restore return address
	
	moveq	#1,%d0
	rts
