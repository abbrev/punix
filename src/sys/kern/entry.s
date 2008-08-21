/*
 * Punix
 * Copyright (C) 2003 PpHd
 * Copyright 2004, 2005 Chris Williams
 * 
 * $Id: entry.s,v 1.13 2008/04/19 18:38:25 fredfoobar Exp $
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

.global buserr
.global SPURIOUS
.global ADDRESS_ERROR
.global ILLEGAL_INSTR
.global ZERO_DIVIDE
.global CHK_INSTR
.global I_TRAPV
.global PRIVILEGE
.global TRACE
.global LINE_1010
.global LINE_1111

.global Int_1
.global Int_2
.global Int_3
.global Int_4
.global Int_5
.global Int_6
.global Int_7

.global _syscall
.global _trap
.global setup_env

|.include "procglo.inc"
.include "signal.inc"

.section _st1,"rx"
.even

/*
 * Bus or Address error exception stack frame:
 *        15     5    4     3    2            0 
 *       +---------+-----+-----+---------------+
 * sp -> |         | R/W | I/N | Function code |
 *       +---------+-----+-----+---------------+
 *       |                 High                |
 *       +- - Access Address  - - - - - - - - -+
 *       |                 Low                 |
 *       +-------------------------------------+
 *       |        Instruction register         |
 *       +-------------------------------------+
 *       |          Status register            |
 *       +-------------------------------------+
 *       |                 High                |
 *       +- - Program Counter - - - - - - - - -+
 *       |                 Low                 |
 *       +-------------------------------------+
 *       R/W (Read/Write): Write = 0, Read = 1.
 *       I/N (Instruction/Not): Instruction = 0, Not = 1.
 */

	.long 0xbeef1001
buserr:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	bus_error
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1002
SPURIOUS:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	spurious
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1003
ADDRESS_ERROR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	address_error
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1004
ILLEGAL_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	illegal_instr
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1005
| send signal SIGFPE
ZERO_DIVIDE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	zero_divide
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1006
CHK_INSTR:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	chk_instr
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1007
I_TRAPV:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	i_trapv
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1008
| send signal SIGILL
PRIVILEGE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	privilege
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef1009
TRACE:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	trace
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

	.long 0xbeef100a
| I don't know (send signal SIGILL?)
| Are there valid 68010+ instructions starting with 1010?
LINE_1010:
	rte

	.long 0xbeef100b
| This should be a floating-point emulator
LINE_1111:
	rte

| Scan for the hardware to know what keys are pressed 
Int_1:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	
	|bsr	KeyScan
	|tst.w	%d4
	|beq.s	0f
	|	move	#0x0001,-(%sp)
	|	move	%d4,-(%sp)
	|	bsr	vtrint
	|	addq.l	#4,%sp
0:

	move	5*4(%sp),-(%sp)		| old ps
	bsr	hardclock
	addq.l	#2,%sp
	
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

/*
 * Does nothing. Why ? Look:
 *  Triggered when the *first* unmasked key (see 0x600019) is *pressed*.
 *  Keeping the key pressed, or pressing another without releasing the first
 *  key, will not generate additional interrupts.  The keyboard is not
 *  debounced in hardware and the interrupt can occasionally be triggered many
 *  times when the key is pressed and sometimes even when the key is released!
 *  So, you understand why you don't use it ;)
 *  Write any value to 0x60001B to acknowledge this interrupt.
 */
Int_2:	move.w	#0x2600,%sr
	move.w	#0x00FF,0x60001A	| acknowledge Int2
oldInt_3:	rte				| Clock for int 3 ?

Int_3:
	bsr	updwalltime
	| addq.l	#1,G+0 | realtime
	rte

| Link Auto-Int
Int_4:
	movem.l	%d0-%d2/%a0-%a1,-(%sp)
	bsr	linkintr	| just call the C routine
	movem.l	(%sp)+,%d0-%d2/%a0-%a1
	rte

| System timers.
Int_5:
	/* FIXME: do timers */
	rte

| ON Int.
|	2ND / DIAMOND : Off
|	ESC : Reset
Int_6:
	/* FIXME: handle ON key */
	rte

| send signal SIGSEGV
Int_7:
	/* FIXME */
	jmp	the_beginning

/*
| SUB FUNCTIONS
| Check the batteries level.	
CheckBatt:
	movem.l	%d1-%d7/%a0-%a6,-(%a7)
	move.w	#0x2500,sr

	| Setup Ptr
	lea	0x600018,%a0
	lea	0x70001C,%a3
	lea	BattTable_HW2(Pc),%a2
	cmpi.b	#1,HW_VERSION
	beq.s	\ok
		lea	BattTable_HW1(Pc),%a2
\ok	
	| Start Checking
	move.w	#0xF,(A3)		| Set HW2 Ports for Batt Check
	moveq	#2,%d2			| 3 times
\loop0
		move.w	#0x380,(A0)	| Setup the minimum trig level
		moveq	#0x52,%d0	| Wait Hardware Answer
		bsr.s	\CheckBattIO	| Loop
		move.w	%d2,%d0
		add.w	%d0,%d0
		move.w	0(%a2,%d0.w),(%a0)
		moveq	#0x6E,%d0
\loop8		btst.b	#2,0x600000
		dbeq	%d0,\loop8
		bne.s	\stop
		dbf	%d2,\loop0	
\stop:	
	addq.w	#1,%d2
	move.w	#7,(%a3)		| Unable Batt Check 1 for HW2 (FIXME: Why ?)
	move.w	#0x380,(A0)		| Setup the minimum trig level
	moveq	#0x52,%d0		| And wait for answer
	bsr.s	\CheckBattIO		| ie restore the standard waiting
	move.w	#6,(%a3)		| Unable Batt Check 2 for HW2

	st.b	%d0			| Flash Rom & Ram Wait States
	cmpi.b	#1,HW_VERSION		| Are only modified on HW1
	bne.s	\end
		move.b	BattWaitStateLevel(Pc,%d2.w),%d0
\end:	move.b	%d0,(0x3-0x18)(%a0)	| Set new Wait States
	move.w	%d2,%d0
	move.b	%d0,BATT_LEVEL
	movem.l	(%a7)+,%d1-%d7/%a0-%a6
	rts

| Battery voltage level is below the trig level if 600000.2=0
\CheckBattIO:
\loop3		btst.b	#2,0x600000
		dbne	%d0,\loop3
	rts

BattWaitStateLevel:	.byte	0xCD,0xDE,0xEF,0xFF
BattTable_HW1:		.word	0x0200,0x0180,0x0100
BattTable_HW2:		.word	0x0200,0x0100,0x0000

_WaitKeyboard:
	moveq	#0x58,%d0
	dbf	%d0,.
	rts

| In: 
|	Nothing
| Out:
|	d4.w = Key
| Destroy:
|	All !
KeyScan:
	lea	0x600018,a0
	lea	0x1B-0x18(a0),a1
	lea	KEY_MASK,a2
	
	| check if a Key is pressed
	clr.w	(a0)			| Read All Keys
	bsr.s	_WaitKeyboard
	move.b	(a1),d0
	not.b	d0
	beq	\NoKey
	| A key is pressed. Check for it.
	| Check which key is pressed
	clr.w	d4
	moveq	#KEY_NBR_ROW-1,d1
	move.w	#KEY_INIT_MASK,d2
\key_loop:
		move.w	d2,(a0)			| Select Row
		bsr.s	_WaitKeyboard
		move.b	(a1),d3			| Read which Keys is pressed
		move.b	d3,d0			| add the Key Mask
		or.b	(a2),d3			| Clear Some Keys according to the mask
		not.b	d0			| UpDate the mask 
		and.b	d0,(a2)+		| 
		not.b	d3
		beq.s	\next			| A key is pressed
			moveq	#7,d0
\bit_loop:			btst	d0,d3
				dbne	d0,\bit_loop
			tst.w	d4		| A key has been already pressed ?
			bne.s	\next
				bset	d0,-1(a2)	| Update Mask so that this key won't be add once more
				move.w	d1,d4
				lsl.w	#3,d4
				add.w	d0,d4
				add.w	d4,d4
				move.w	Translate_Key_Table(Pc,d4.w),d4
				move.w	d4,KEY_PREVIOUS
				move.w	d2,KEY_CUR_ROW	| Memorize which key is currently pressed
				move.w	d0,KEY_CUR_COL
				move.w	KEY_ORG_START_CPT,KEY_CPT	| Start Delay before repeat it
\next:		ror.w	#1,d2
		dbf	d1,\key_loop		
	| Auto Repeat Feature FIXME: It seems not to work well. Why ?
	tst.w	d4		| Is a key pressed ?
	bne.s	\end		| Yes do not read the previous key.
		cmp.w	#0x1000,KEY_PREVIOUS	| No repeat feature for statut keys
		bge.s	\none
		| No so check is previous key is still pressed.
		move.w	KEY_CUR_ROW,(a0)	| Select Row
		bsr	_WaitKeyboard
		move.b	(a1),d3			| Read which Keys is pressed
		move.w	KEY_CUR_COL,d0
		btst	d0,d3			| Previous Key is not pressed
		bne.s	\ResetStatutKeys
			subq.w	#1,KEY_CPT	| Dec cpt.
			bne.s	\end
				move.w	KEY_ORG_REPEAT_CPT,KEY_CPT
				move.w	KEY_PREVIOUS,d4
				bra.s	\end
\NoKey:	
	clr.l	(a2)+		| Reset KEY_MASK
	clr.l	(a2)+
	clr.w	(a2)+
\none:	clr.w	d4
\end:	move.w	#0x380,(a0)	| Reset to standard Key Reading.
	rts
\ResetStatutKeys:
		and.b	#RESET_KEY_STATUS_MASK,KEY_MASK+KEY_NBR_ROW-1		| Clear Mask
		bra	KeyScan
	
.ifdef	TI89		| For TI-89
Translate_Key_Table:
	dc.w	KEY_UP,KEY_LEFT,KEY_DOWN,KEY_RIGHT,KEY_2ND,KEY_SHIFT,KEY_DIAMOND,KEY_ALPHA
	dc.w	KEY_ENTER,'+','-','*','/','^',KEY_CLEAR,KEY_F5
	dc.w	KEY_SIGN,'3','6','9',',','t',KEY_BACK,KEY_F4
	dc.w	'.','2','5','8',')','z',KEY_CATALOG,KEY_F3
	dc.w	'0','1','4','7','(','y',KEY_MODE,KEY_F2
	dc.w	KEY_APPS,KEY_STO,KEY_EE,KEY_OR,'=','x',KEY_HOME,KEY_F1
	dc.w	KEY_ESC,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID	
.else			| For TI-92+ and V200
Translate_Key_Table:
	dc.w	KEY_2ND,KEY_DIAMOND,KEY_SHIFT,KEY_HAND,KEY_LEFT,KEY_UP,KEY_RIGHT,KEY_DOWN
	dc.w	KEY_VOID,'z','s','w',KEY_F8,'1','2','3'
	dc.w	KEY_VOID,'x','d','e',KEY_F3,'4','5','6'
	dc.w	KEY_STO,'c','f','r',KEY_F7,'7','8','9'
	dc.w	' ','v','g','t',KEY_F2,'(',')',','
	dc.w	'/','b','h','y',KEY_F6,KEY_SIN,KEY_COS,KEY_TAN
	dc.w	'^','n','j','u',KEY_F1,KEY_LN,KEY_ENTER,'p'
	dc.w	'=','m','k','i',KEY_F5,KEY_CLEAR,KEY_APPS,'*'
	dc.w	KEY_BACK,KEY_THETA,'l','o','+',KEY_MODE,KEY_ESC,KEY_VOID
	dc.w	'-',KEY_ENTER,'a','q',KEY_F4,'0','.',KEY_SIGN
.endif

	| UpDate the Key buffer (FIFO Buffer)
UpDateKeyBuffer:
	move.w	KEY_CUR_POS,%d3
	beq.s	9f				| No Key in Buffer
	tst.w	TEST_PRESSED_FLAG		| Key has not been readen by apps.
	bne.s	9f
		| Move Key Buffer : remove the last Key
		clr.w	%d0
		lea	GETKEY_CODE,%a3
		subq.w	#1,%d3			| Remove a key from Buffer
		move.w	%d3,KEY_CUR_POS		| Save new value
		beq.s	0f			| = 0 ?
			moveq	#2,%d0		| No, so a key is in the buffer
0:		move.w	%d0,TEST_PRESSED_FLAG	| 2 so that OSdqueue(kbd_queue) works fine.
0:			move.w	2(%a3),(%a3)+
			dbf	%d3,0b
9:
	rts

| Add a key in the keyboard FIFO buffer.
|	KEY_2ND, KEY_SHIFT, KEY_DIAMOND and KEY_ALPHA are treated in a special way.
| In:
|	In d4.w = code (<> 0 !)
| This code MUST work :
|	tst.w	KEY_PRESSED_FLAG	| has a key been pressed?
|	beq	wait_idle
|	move.w	GETKEY_CODE,d0
|	clr.w	KEY_PRESSED_FLAG	| clear key buffer
AddKey:
	move.w	KEY_STATUS,d3		| Read Statut Key
	cmp.w	#0x1000,d4		| Is it a normal key or an option key ?
	bcs.s	\normal_key
.ifdef TI89
		clr.b	d1
		cmp.w	#KEY_ALPHA,d4
		bne.s	\NoAlphaStatKey
			| Check if KEY_MAJ is set
			tst.b	KEY_MAJ
			beq.s	\CheckExtraCombos
				clr.w	d4
				bra.s	\NoShiftAlphaCombo
\CheckExtraCombos	| Check Alpha-Alpha Combo 
			cmp.w	#KEY_ALPHA,d3
			bne.s	\NoAlphaAlphaCombo
				moveq	#2,d1
				clr.w	d4
\NoAlphaAlphaCombo	| Check Shift-Alpha Combo 
			cmp.w	#KEY_SHIFT,d3
			bne.s	\NoShiftAlphaCombo
				moveq	#1,d1
				clr.w	d4				
\NoShiftAlphaCombo
		move.b	d1,KEY_MAJ
\NoAlphaStatKey
.endif
		cmp.w	d3,d4	| Option Key: update the status.
		bne.s	\Ok
			clr.w	d4	| Erase the statut if we pressed twice the same option key.
\Ok:		move.w	d4,KEY_STATUS	| Re KeyScan ?
		rts
\normal_key:
	cmpi.w	#KEY_SHIFT,d3		| Shift Key
	beq.s	\shift	
	cmpi.w	#KEY_DIAMOND,d3		| Diamond Key
	beq.s	\diamond
.ifdef TI89
	cmpi.w	#KEY_ALPHA,d3		| Ti89 only : Alpha Keys
	beq.s	\alpha	
.endif
	cmpi.w	#KEY_2ND,d3		| 2nd Key
	bne.s	\normal
	
\2nd:
.ifndef TI89
	cmpi.w	#'z',d4			| 2nd + Z only alvailable on 92+/v200
	bne.s	\no_exg
		not.b	KEY_MAJ		| 2nd + Z
		bra.s	\overflow
\no_exg	
.endif
	lea	Translate_2nd(pc),a0	| Translate 2nd Keys
\Loop2nd	move.w	(a0)+,d0
		beq.s	\extended
		addq.l	#2,a0
		cmp.w	d0,d4
		bne.s	\Loop2nd
	move.w	-(a0),d4
	bra.s	\add_key

.ifdef TI89
\alpha	bsr	TranslateAlphaKey	| Translate Alpha Key
	bra.s	\add_key
.endif

\diamond:
	| Test '+' / '-'
	cmpi.w	#'+',d4
	bne.s	\NoContrastUp	
		clr.w	KEY_STATUS				| Clear statut
		jmp	OSContrastUp
\NoContrastUp
	cmpi.w	#'-',d4
	bne.s	\3rdCont
		clr.w	KEY_STATUS				| Clear statut
		jmp	OSContrastDn
\3rdCont
.ifdef TI89
	lea	Translate_3rd(pc),a0	| Translate 3rd Keys
\Loop3rd	move.w	(a0)+,d0
		beq.s	\extended
		addq.l	#2,a0
		cmp.w	d0,d4
		bne.s	\Loop3rd
	move.w	-(a0),d4
	bra.s	\add_key
.endif
\extended				| Only or KEY_STATUS and KEY
	or.w	d3,d4
	bra.s	\add_key

\shift:					| SHIFT called
.ifdef TI89
	bsr	TranslateAlphaKey	| Translate alpha Key
.endif
	cmpi.w	#127,d4			| Check if range Ok.
	bhi.s	\extended		| No so extended
	bra.s	\MAJ			| Go to upper case
	
\normal:				| Normal Key
	move.b	KEY_MAJ,d1		
	beq.s	\add_key
.ifdef	TI89
		bsr.s	TranslateAlphaKey
		subq.b	#2,d1
		beq.s	\add_key
.endif
\MAJ:	
	cmpi.w	#'a',d4
	bcs.s	\add_key
	cmpi.w	#'z',d4
	bhi.s	\add_key
		addi.w	#'A'-'a',d4

\add_key:
	bsr.s	AddKeyToFIFOKeyBuffer	
\overflow:
	clr.w	KEY_STATUS				| Clear statut
	rts

.ifdef TI89
TranslateAlphaKey:
	lea	Translate_Alpha(%pc),%a0
0:		move.b	(%a0)+,%d0
		beq.s	0f
		addq.l	#1,%a0
		cmp.b	%d0,%d4
		bne.s	0b
	clr.w	%d4
	move.b	-(%a0),%d4
0:	rts
.endif

| In : d4.w	
AddKeyToFIFOKeyBuffer:
	move.w	KEY_CUR_POS,d3			| Current position in Buffer
	cmpi.w	#KEY_MAX,d3			| Max size of buffer
	bcc.s	0f
		lea	GETKEY_CODE,a3		| Ptr to buffer
		adda.w	d3,a3			| d3*2
		move.w	d4,0(a3,d3.w)		| Write it to buffer
		addq.w	#1,d3			| One more
		move.w	d3,KEY_CUR_POS		| Save new position
		move.w	#2,TEST_PRESSED_FLAG	| A key has been pressed
0:
	rts

|	First Key is the source and then the new key
.ifdef TI89

Translate_2nd:
	.word	KEY_F1,KEY_F6
	.word	KEY_F2,KEY_F7
	.word	KEY_F3,KEY_F8
	.word	KEY_ESC,KEY_QUIT
	.word	KEY_APPS,KEY_SWITCH
	.word	KEY_HOME,KEY_CUSTOM
	.word	KEY_MODE,26
	.word	KEY_CATALOG,151
	.word	KEY_BACK,KEY_INS
	.word	'x',KEY_LN
	.word	'y',KEY_SIN
	.word	'z',KEY_COS
	.word	't',KEY_TAN
	.word	'^',128+12
	.word	'=',39
	.word	'(','{'
	.word	')','}'
	.word	',','['
	.word	'/',']'
	.word	'|',176
	.word	'7',176+13
	.word	'8',176+12
	.word	'9',';'
	.word	'*',168
	.word	KEY_EE,159
	.word	'4',':'
	.word	'5',KEY_MATH
	.word	'6',KEY_MEM
	.word	'-',KEY_VARLINK
	.word	KEY_STO,KEY_RCL
	.word	'1','"'
	.word	'2','\'
	.word	'3',KEY_UNITS
	.word	'+',KEY_CHAR
	.word	'0','<'	
	.word	'.','>'
	.word	KEY_SIGN,KEY_ANS
	.word	KEY_ENTER,KEY_ENTRY
	.word	KEY_CLEAR,'$'
	.word	0
Translate_3rd:
	.word	KEY_MODE,'_'
	.word	KEY_CATALOG,190
	|.word	'x',KEY_EXP	| Pb with side cut
	.word	'^',KEY_THETA
	.word	'=',KEY_DIFERENT
	.word	KEY_CLEAR,'%'
	.word	'/','!'
	.word	'*','&'
	.word	KEY_STO,'@'
	.word	'0',KEY_INFEQUAL
	.word	'.',KEY_SUPEQUAL
	.word	KEY_ENTER,KEY_ENTRY
	.word	'(','#'
	.word	')',18
	.word	',','?'
	.word	'|',191
	.word	'7',176
	.word	'8',159
	.word	'9',169
	.word	'4',128
	.word	'5',129
	.word	'6',130
	.word	'1',131
	.word	'2',132
	.word	'3',133
	.word	KEY_SIGN,96
	.word	0
Translate_Alpha:
	.byte	'=','a'
	.byte	'(','b'
	.byte	')','c'
	.byte	',','d'
	.byte	'/','e'
	.byte	'|','f'
	.byte	'7','g'
	.byte	'8','h'
	.byte	'9','i'
	.byte	'*','j'
	.byte	KEY_EE,'k'
	.byte	'4','l'
	.byte	'5','m'
	.byte	'6','n'
	.byte	'-','o'
	.byte	KEY_STO,'p'
	.byte	'1','q'
	.byte	'2','r'
	.byte	'3','s'
	.byte	'+','u'
	.byte	'0','v'
	.byte	'.','w'
	.byte	KEY_SIGN,' '
	.byte	'x','x'
	.byte	'y','y'
	.byte	'z','z'
	.byte	't','t'	
	.byte	0

.else

Translate_2nd:
	.word	'q','?'
	.word	'w','!'
	.word	'e','é'
	.word	'r','@'
	.word	't','#'
	.word	'y',26
	.word	'u',252
	.word	'i',151
	.word	'o',212
	.word	'p','_'
	.word	'a','à'
	.word	's',129
	.word	'd',176
	.word	'f',159
	.word	'g',128
	.word	'h','&'
	.word	'j',190
	.word	'k','|'
	.word	'l','"'
	.word	'x',169
	.word	'c',199
	.word	'v',157
	.word	'b',39
	.word	'n',241
	.word	'm',';'
	.word	'=','\'
	.word	KEY_THETA,':'
	.word	'(','{'
	.word	')','}'
	.word	',','['
	.word	'/',']'
	.word	'^',140
	.word	'7',189
	.word	'8',188
	.word	'9',180
	.word	'*',168
	.word	'4',142
	.word	'5',KEY_MATH	|171
	.word	'6',KEY_MEM	|187
	.word	'-',KEY_VARLINK	|143
	.word	'1',KEY_EE	|149
	.word	'2',KEY_CATALOG	|130
	.word	'3',KEY_CUSTOM	|131
	.word	'+',KEY_CHAR	|132
	.word	'0','<'
	.word	'.','>'
	.word	KEY_SIGN,KEY_ANS	|170
	.word	KEY_BACK,KEY_INS
	.word	KEY_ENTER,KEY_ENTRY
	.word	KEY_APPS,KEY_SWITCH
	.word	KEY_ESC,KEY_QUIT
	.word	KEY_STO,KEY_RCL
	.word	' ','$'
	.word	0

.endif
	
	EVEN
*/

| Assembly interface for C version; prototype of C version:
| uint32_t syscall(unsigned callno, void **usp, short *sr)
_syscall:
	movem.l	%d3-%d7/%a2-%a6,-(%sp)	| XXX: this is needed for vfork!
	
	lea.l	10*4(%sp),%a0
	move.l	%a0,-(%sp)	| short *sr
	
	move.l	%usp,%a0
	move.l	%a0,-(%sp)	| void **usp
	
	move	%d0,-(%sp)	| int callno
	
	bsr	syscall
	adda.l	#10,%sp
	
	movem.l	(%sp)+,%d3-%d7/%a2-%a6
	rte

| jmp_buf
.equ jmp_buf.reg,0
.equ jmp_buf.sp,10*4
.equ jmp_buf.usp,11*4
.equ jmp_buf.retaddr,12*4

| struct trapframe
.equ trapframe.sr,0
.equ trapframe.pc,2

/* void setup_env(jmp_buf env, struct trapframe *tfp, long *sp);
 * Setup the execution environment "env" using the trapframe "tfp", the stack
 * pointer "sp", and the current user stack pointer. The trapframe is pushed
 * onto the stack (*sp), and the new stack pointer is saved in the environment.
 * FIXME: make this routine cleaner and more elegant!
 */
setup_env:
	move.l	12(%sp),%a0		| %a0 = sp
	move.l	8(%sp),%a1		| %a1 = tfp
	
	| setup the new trap frame
	move.l	trapframe.pc(%a1),-(%a0)	| push pc
	clr	-(%a0)				| clear sr
	
	move.l	4(%sp),%a1		| %a1 = env
	move.l	%a0,jmp_buf.sp(%a1)	| sp
	
	move.l	%usp,%a0
	move.l	%a0,jmp_buf.usp(%a1)	| usp
	
	move.l	1f(%pc),jmp_buf.retaddr(%a1)
	
	move.l	8(%sp),%a0		| %a0 = tfp
	move	#10-1,%d0
0:	move.l	-(%a0),(%a1)+		| copy the saved regs to the child's env
	dbra	%d0,0b
	
	rts

/* the child process resumes here  */
1:
	move	#0,%d0	| return 0 to the child process
	rte

| unused for now
_trap:
	rte
