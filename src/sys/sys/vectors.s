|.include "vectors.inc"

/* Vectors table */
.section _stl2,"rx"
.global vectors_table
vectors_table:
	.long	0x4c00			| 00 Stack Ptr
	.long	the_beginning		| 04 Start of Kernel
	.long	buserr		| 08 Bus Error
	.long	ADDRESS_ERROR		| 0C Address Error
	.long	ILLEGAL_INSTR		| 10 Illegal Instruction
	.long	ZERO_DIVIDE		| 14 Divide by zero
	.long	CHK_INSTR		| 18 CHK Instruction
	.long	I_TRAPV			| 1C TRAPV
	.long	PRIVILEGE		| 20 Privilege Violation
	.long	TRACE			| 24 Trace
	.long	LINE_1010		| 28 Line 1010 Emulator
	.long	fpuemu			| 2C Line 1111 Emulator
	.word	0,0			| 30 Kernel Version / Name
	.long	0			| 34
	.long	0			| 38
	.long	0			| 3C
	.long	0			| 40
	.long	0			| 44
	.long	0x00			| 48
	.long	0x00			| 4C
	.long	0			| 50
	.long	0x00			| 54 Unused
	.long	0x00			| 58 Unused
	.long	0x00			| 5C Unused
	.long	SPURIOUS		| 60 Spurious Interrupt
	.long	Int_1			| 64 Auto-Int 1
	.long	Int_2			| 68 Auto-Int 2
	.long	Int_3			| 6C Auto-Int 3
	.long	Int_4			| 70 Auto-Int 4
	.long	Int_5			| 74 Auto-Int 5
	.long	Int_6			| 78 Auto-Int 6
	.long	Int_7			| 7C Auto-Int 7
	.long	_syscall		| 80 trap 0
	.long	TRAP_1			| 84 trap 1
	.long	TRAP_2			| 88 trap 2
	.long	TRAP_3			| 8C trap 3
	.long	TRAP_4			| 90 trap 4
	.long	TRAP_5			| 94 trap 5
	.long	TRAP_6			| 98 trap 6
	.long	TRAP_7			| 9C trap 7
	.long	TRAP_8			| A0 trap 8
	.long	TRAP_9			| A4 trap 9
	.long	TRAP_10			| A8 trap 10
	.long	TRAP_11			| AC trap 11
	.long	TRAP_12			| B0 trap 12
	.long	TRAP_13			| B4 trap 13
	.long	TRAP_14			| B8 trap 14
	.long	TRAP_15			| BC trap 15
	.long	0xFF0055AA		| C0 *SIGNATURE
	.long	0x1FFFFC		| C4 *RAM_SIZE
	.fill	0x100-0x00C8		| XXX: why was it 0x400 previously??
/*
	.long	0 |ROMCALLS_TABLE	| C8 *ROM_CALL_TABLE
	.long	0,0,0,0			| CC *Users Vectors
	.long	0,0,0,0			| D0 *
	.long	0,0,0,0			| D4 *
	.long	0			| D8 *
*/
End_Vectors_Table:
