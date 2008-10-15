.include "cell.inc"

.section _st1, "rx"

|void scrolldown(int rows)
|scroll terminal window down (text moves up)
|tested and works
.global scrolldown
scrolldown:
	move	4(%sp),%d1
	beq.s	9f
	
	mulu	#LCD_INCY*6,%d1
	lea.l	LCD_MEM,%a0	| dest
	move	#NUMCELLROWS*6*LCD_INCY,%d0
	sub	%d1,%d0
	ext.l	%d0
	
	lea.l	0(%a0,%d1),%a1	| src
	bsr	memmove_reg
	
	asr	#2,%d1
	subq.l	#1,%d1
	lea.l	LCD_MEM+NUMCELLROWS*6*LCD_INCY,%a0
0:	clr.l	-(%a0)
	dbra	%d1,0b
	
9:	rts

|void scrollup(int rows)
|scroll terminal window up (text moves down)
|not tested!
.global scrollup
scrollup:
	move	4(%sp),%d1
	beq.s	9f
	
	mulu	#LCD_INCY*6,%d1
	lea.l	LCD_MEM,%a1	| src
	move	#NUMCELLROWS*6*LCD_INCY,%d0
	sub	%d1,%d0
	ext.l	%d0
	
	lea.l	0(%a1,%d1),%a0	| dest
	bsr	memmove_reg
	
	asr	#2,%d1
	subq.l	#1,%d1
	lea.l	LCD_MEM,%a0
0:	clr.l	(%a0)+
	dbra	%d1,0b
	
9:	rts
