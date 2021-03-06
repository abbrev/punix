.include "cell.inc"

.section .text

|void scrolldown(int rows, void *lcd)
|scroll terminal window down (text moves up)
|tested and works
.global scrolldown
scrolldown:
	move	4(%sp),%d1
	beq	9f
	move.l	6(%sp),%a0
	
	mulu	#LCD_INCY*6,%d1
	|lea.l	LCD_MEM,%a0	| dest
	move	#NUMCELLROWS*6*LCD_INCY,%d0
	sub	%d1,%d0
	ext.l	%d0
	
	lea.l	0(%a0,%d1),%a1	| src
	jbsr	memmove_reg
	
	asr	#2,%d1
	subq.l	#1,%d1
	move.l	6(%sp),%a0
	add	#NUMCELLROWS*6*LCD_INCY,%a0
	|lea.l	LCD_MEM+NUMCELLROWS*6*LCD_INCY,%a0
0:	clr.l	-(%a0)
	dbra	%d1,0b
	
9:	rts

|void scrollup(int rows, void *lcd)
|scroll terminal window up (text moves down)
|not tested!
.global scrollup
scrollup:
	move	4(%sp),%d1	| d1 = rows
	beq	9f
	move.l	6(%sp),%a1	| a1 = lcd
	
	mulu	#LCD_INCY*6,%d1
	|lea.l	LCD_MEM,%a1	| src
	move	#NUMCELLROWS*6*LCD_INCY,%d0
	sub	%d1,%d0
	ext.l	%d0
	
	lea.l	0(%a1,%d1),%a0	| dest
	jbsr	memmove_reg
	
	asr	#2,%d1
	subq.l	#1,%d1
	move.l	6(%sp),%a0
	|lea.l	LCD_MEM,%a0
0:	clr.l	(%a0)+
	dbra	%d1,0b
	
9:	rts

/*
| void memmove_reg(void *dest, const void *src, size_t count);
memmove_reg:
	cmp.l	%a0,%a1		| Src <= dest
	bls	1f
0:		move.b	(%a1)+,(%a0)+
		subq.l	#1,%d0
		bne	0b
	rts
1:
	add.l	%d0,%a0
	add.l	%d0,%a1
0:		move.b	-(%a1),-(%a0)
		subq.l	#1,%d0
		bne	0b
	rts
	*/
