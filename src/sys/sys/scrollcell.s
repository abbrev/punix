.include "cell.inc"

.section _st1, "x"

|void scrollcellup(int rows)
.global scrollcellup
scrollcellup:
	move	4(%sp),%d0
	beq.s	9f
	
	mulu	#LCD_INCY*6,%d0
	move	%d0,4(%sp)
	lea.l	LCD_MEM,%a0
	move	#NUMCELLROWS*6*LCD_INCY,%d1
	sub	%d0,%d1
	ext.l	%d1
	
	|move.l	%d1,-(%sp)
	|pea.l	0(%a0,%d0)
	|pea.l	LCD_MEM
	|bsr	memmove
	|lea.l	12(%sp),%sp
	
	lea.l	0(%a0,%d0),%a1
	lea.l	LCD_MEM,%a0
	move.l	%d1,%d0
	bsr	memmove_reg
	
	move	4(%sp),%d0
	subq.l	#1,%d0
	lea.l	LCD_MEM+NUMCELLROWS*6*LCD_INCY,%a0
0:	clr.b	-(%a0)
	dbra	%d0,0b
	
9:	rts

|void scrollcelldown(int rows)
.global scrollcelldown
scrollcelldown:
	move	4(%sp),%d0
	beq.s	9f
	
	/* FIXME: write this! */
	
	nop
9:	rts
