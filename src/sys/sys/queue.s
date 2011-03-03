.equ QSIZE, 512	/* must be a power of 2. also see h/queue.h */
.equ QMASK, QSIZE-1

.equ q_count, 0
.equ q_head, 2
.equ q_tail, 4
.equ q_buf, 6

/* void qclear(struct queue *qp); */
	.global	qclear
qclear:
	move.l	4(%sp),%a0	/* qp */
	clr	(%a0)+
	clr	(%a0)+
	clr	(%a0)+
	rts

/* int putc(int ch, struct queue *qp); */
	.global	putc
putc:
	move	%sr,-(%sp)
	move	#0x2500,%sr
	
	move.l	6+2(%sp),%a0	/* qp */
	cmp	#QSIZE,q_count(%a0)
	beq	8f
	
	move	6+0(%sp),%d0	/* ch */
	move	q_head(%a0),%d1
	lea.l	q_buf(%a0),%a1
	move.b	%d0,(%d1.w,%a1)
	moveq	#0,%d0
	addq	#1,%d1
	and	#QMASK,%d1
	move	%d1,q_head(%a0)
	add	#1,q_count(%a0)
	
9:	move	(%sp)+,%sr
	rts
8:	moveq	#-1,%d0
	bra	9b

/* int unputc(struct queue *qp); */
	.global	unputc
unputc:
	move	%sr,-(%sp)
	move	#0x2500,%sr
	
	move.l	6+0(%sp),%a0
	tst	q_count(%a0)
	beq	8f
	
	
	move	q_head(%a0),%d1
	lea.l	q_buf(%a0),%a1
	subq	#1,%d1
	and	#QMASK,%d1
	move	%d1,q_head(%a0)
	move.b	(%d1.w,%a1),%d0
	and	#0xff,%d0
	sub	#1,q_count(%a0)
	
9:	move	(%sp)+,%sr
	rts
8:	moveq	#-1,%d0
	bra	9b

/* int getc(struct queue *qp); */
	.global	getc
getc:
	move	%sr,-(%sp)
	move	#0x2500,%sr
	
	move.l	6+0(%sp),%a0
	tst	q_count(%a0)
	beq	8f
	
	
	move	q_tail(%a0),%d1
	lea.l	q_buf(%a0),%a1
	move.b	(%d1.w,%a1),%d0
	and	#0xff,%d0
	addq	#1,%d1
	and	#QMASK,%d1
	move	%d1,q_tail(%a0)
	sub	#1,q_count(%a0)
	
9:	move	(%sp)+,%sr
	rts
8:	moveq	#-1,%d0
	bra	9b

/* int ungetc(int ch, struct queue *qp); */
	.global	ungetc
ungetc:
	move	%sr,-(%sp)
	move	#0x2500,%sr
	
	move.l	6+2(%sp),%a0	/* qp */
	cmp	#QSIZE,q_count(%a0)
	beq	8f
	
	move	6+0(%sp),%d0	/* ch */
	move	q_tail(%a0),%d1
	lea.l	q_buf(%a0),%a1
	subq	#1,%d1
	and	#QMASK,%d1
	move	%d1,q_tail(%a0)
	move.b	%d0,(%d1.w,%a1)
	moveq	#0,%d0
	add	#1,q_count(%a0)
	
9:	move	(%sp)+,%sr
	rts
8:	moveq	#-1,%d0
	bra	9b

