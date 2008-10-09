#ifdef TI89
#define IS89_0_2		 0
#define KEY_ESC_ROW		 0x3f
#define KEY_ESC_COL		 0
#define KEY_APPS_ROW		 0x5f
#define KEY_APPS_COL		 0
#define KEY_NBR_ROW		 7	; 10 for 92+/v200, 7 for 89
/*#define KEY_INIT_MASK		 0xFFBF	; $FDFF for 92+/89 $FFBF for 89*/
#define RESET_KEY_STATUS_MASK	 0x0F
#define SCR_WIDTH		 160
#define SCR_HEIGHT		 100
#define USED_FONT		 0	; Small Font for dialog
#define TAB_SIZE		 40
#elif defined(TI92P)
#define IS89_0_2		 2
#define KEY_ESC_ROW		 0x2ff
#define KEY_ESC_COL		 6
#define KEY_APPS_ROW		 0x37f
#define KEY_APPS_COL		 6
#define KEY_NBR_ROW		 10	; 10 for 92+/v200, 7 for 89
/*#define KEY_INIT_MASK		 $FDFF	; $FDFF for 92+/89 $FFBF for 89*/
#define RESET_KEY_STATUS_MASK	 0xF0
#define SCR_WIDTH		 240
#define SCR_HEIGHT		 128
#define USED_FONT		 1	; Normal Font for dialog
#define TAB_SIZE		 60
#endif

#define KEY_MAX	 20

#ifdef TI92P
#define KEY_LEFT	 337
#define KEY_UP		 338
#define KEY_RIGHT	 340
#define KEY_DOWN	 344
#define KEY_CATALOG	 4146
#elif defined(TI89)
#define KEY_LEFT	 338
#define KEY_UP		 337
#define KEY_RIGHT	 344
#define KEY_DOWN	 340
#define KEY_CATALOG	 $116
#endif

#define KEY_VOID	 0
#define KEY_ENTER	 13
#define KEY_THETA	 136
#define KEY_SIGN	 173
#define KEY_BACK	 257
#define KEY_STO		 22	; Original Key code is 258
#define KEY_COS		 260
#define KEY_TAN		 261
#define KEY_LN		 262
#define KEY_CLEAR	 263
#define KEY_ESC		 264
#define KEY_APPS	 265
#define KEY_MODE	 266
#define KEY_F1		 268
#define KEY_F2		 269
#define KEY_F3		 270
#define KEY_F4		 271
#define KEY_F5		 272
#define KEY_F6		 273
#define KEY_F7		 274
#define KEY_F8		 275
#define KEY_SIN		 259
#define KEY_INS		 4353
#define KEY_ON		 267
#define KEY_OFF		 4363
#define KEY_OFF2	 16651
#define KEY_QUIT	 4360
#define KEY_SWITCH	 4361
#define KEY_VARLINK	 4141
#define KEY_CHAR	 4139
#define KEY_ENTRY	 4109
#define KEY_RCL		 4354
#define KEY_MATH	 4149
#define KEY_MEM		 4150
#define KEY_ANS		 4372
#define KEY_CUSTOM	 4147
#define KEY_UNITS	 4400
#define KEY_2ND		 0x1000
#define KEY_DIAMOND	 0x2000
#define KEY_SHIFT	 0x4000
#define KEY_HAND	 0x8000
#define KEY_ALPHA	 0x8000

#define KEY_INFEQUAL	 156
#define KEY_DIFERENT	 157
#define KEY_SUPEQUAL	 158

#define KEY_EE		 149
#define KEY_HOME	 277
#define KEY_OR		 124

#define KEY_AUTO_REPEAT	 0x0800

	ifd	PEDROM_89
HELPKEYS_KEY		EQU	KEY_DIAMOND|KEY_EE
	endif
	ifd	PEDROM_92
HELPKEYS_KEY		EQU	KEY_DIAMOND|'k'
	endif

LINE_FEED	EQU	13

**********************************************************
HANDLE_MAX	EQU	2000

**********************************************************
SYM_ENTRY.name equ 0
SYM_ENTRY.compat equ 8
SYM_ENTRY.flags equ 10
SYM_ENTRY.hVal equ 12
SYM_ENTRY.sizeof equ 14

SF_GREF1 equ $0001
SF_GREF2 equ $0002 
SF_STATVAR equ $0004 
SF_LOCKED equ $0008 
SF_HIDDEN equ $0010 
SF_OPEN equ $0010 
SF_CHECKED equ $0020 
SF_OVERWRITTEN equ $0040 
SF_FOLDER equ $0080 
SF_INVIEW equ $0100 
SF_ARCHIVED equ $0200 
SF_TWIN equ $0400 
SF_COLLAPSED equ $0800 
SF_LOCAL equ $4000 
SF_BUSY equ $8000

FOLDER_LIST_HANDLE	EQU	8		; To be compatible with AMS 1.0x
MAIN_LIST_HANDLE	EQU	9		; To be compatible with AMS 1.0x

FO_SINGLE_FOLDER	EQU	$01
FO_RECURSE 		EQU	$02
FO_SKIP_TEMPS		EQU	$04
FO_RETURN_TWINS 	EQU	$08
FO_RETURN_FOLDER 	EQU	$10
FO_SKIP_COLLAPSE 	EQU	$20

STOF_ESI		EQU	$4000
STOF_ELEMENT		EQU	$4001
STOF_NONE		EQU	$4002
STOF_HESI		EQU	$4003

**********************************************************
ESTACK_SIZE		EQU	700			; Fixed size of the EStack
ESTACK_HANDLE		EQU	1			; Handle of the EStack

**********************************************************
END_ARCHIVE		EQU	ROM_BASE+ROM_SIZE	; End of the archive memory

; Description of an archive entry in the archive memory.
ARC_ENTRY.status	EQU	-20			; Status (ARC_ST type)
ARC_ENTRY.folder	EQU	-18			; Folder name (8 bytes)
ARC_ENTRY.name		EQU	-10			; Entry name (8 bytes)
ARC_ENTRY.checksum	EQU	-2			; Checksum of the entry
ARC_ENTRY.file		EQU	0			; The file itself
ARC_ENTRY.HeaderSize	EQU	20			; Size of the header of an entry

ARC_ST_VOID		EQU	$FFFF			; Unused entry
ARC_ST_WRITTING         EQU     $FFFE                   ; Entry currently written
ARC_ST_INUSE		EQU	$FFFC			; Used entry
ARC_ST_DELETED		EQU	$FFF8			; Deleted entry

APP_UNUSED_BSS_SIZE	EQU	4

ARG_MAX			EQU	35

CertificateMemory	EQU	ROM_BASE+$10000
FlashMemoryEnd		EQU	END_ARCHIVE

**********************************************************
QUEUE.head		EQU	0
QUEUE.tail		EQU	2
QUEUE.size		EQU	4
QUEUE.used		EQU	6
QUEUE.data		EQU	8

LINK_QUEUE.sizeof	EQU	250
LINK_MAX_WAIT		EQU	10*20		; 10s max.

PACKET.mid		EQU	0
PACKET.cid		EQU	1
PACKET.len		EQU	2
PACKET.data		EQU	4

CID_VAR			EQU	$06 	; Includes a std variable header (used in receiving)
CID_CTS			EQU	$09 	; Used to signal OK to send a variable               |
CID_XDP			EQU	$15 	; Xmit Data Packet (pure data)                       |
CID_SKIP		EQU	$36 	; Skip/exit - used when duplicate name is found      |
CID_ACK			EQU	$56 	; Acknowledgment                                     |
CID_ERR			EQU	$5A	; Checksum error: send last packet again             |
CID_RDY			EQU	$68 	; Check whether TI is ready                          |
CID_SCR			EQU	$6D 	; Request screenshot                                 |
CID_CMD			EQU	$87 	; direct CMD | Direct command (for remote control for instance)   |
CID_EOT			EQU	$92 	; EOT/DONE   | End Of Transmission: no more variables to send     |
CID_REQ			EQU	$A2 	; Request variable - includes a standard variable    |
CID_RTS			EQU	$C9	; Request to send - includes a padded variable header|

TY_NORMAL		EQU	$1D	; 
TY_LOCKED		EQU	$26	;
TY_ARCHIVED		EQU	$27	;
	
FILE_SIZE		EQU	10	; sizeof(FILE)
MAX_FILES		EQU	10	; Must be changed in PedroM internal too!

**********************************************************
WINDOW.Flags		EQU	0	;/* Window flags */ 
WINDOW.CurFont		EQU	2	; unsigned char CurFont; /* Current font */ 
WINDOW.CurAttr		EQU	3	; unsigned char CurAttr; /* Current attribute */ 
WINDOW.Background	EQU	4	; unsigned char Background; /* Current background attribute */ 
WINDOW.TaskId		EQU	6	; short TaskId; /* Task ID of owner */ 
WINDOW.CurX		EQU	8	; short CurX
WINDOW.CurY		EQU	10	; short CurY; /* Current (x,y) position (relative coordinates) */ 
WINDOW.CursorX		EQU	12	; short CursorX
WINDOW.CursorY		EQU	14	; short CursorY; /* Cursor (x,y) position */ 
WINDOW.Client		EQU	16	; SCR_RECT Client; /* Client region of the window (excludes border) */ 
WINDOW.Window		EQU	20	; SCR_RECT Window; /* Entire window region including border */ 
WINDOW.Clip		EQU	24	; SCR_RECT Clip; /* Current clipping region */ 
WINDOW.Port		EQU	28	; SCR_RECT Port; /* Port region for duplicate screen */ 
WINDOW.DupScr		EQU	34	; unsigned short DupScr; /* Handle of the duplicated or saved screen area */ 
WINDOW.Next		EQU	36	; struct WindowStruct *Next; /* Pointer to the next window in the linked list */ 
WINDOW.Title		EQU	40	; char *Title; /* Pointer to the (optional) title */ 
WINDOW.savedScrState	EQU	44	; SCR_STATE savedScrState; /* Saved state of the graphics system */ 
WINDOW.Screen		EQU	48	; 

WF_SAVE_SCR		EQU	$10
WF_DUP_SCR		EQU	$20
WF_TTY			EQU	$40
WF_NOBOLD		EQU	$200
WF_NOBORDER		EQU	$100
WF_ROUNDEDBORDER	EQU	$8
WF_TITLE		EQU	$1000
WF_BLACK		EQU	$800

;WF_VIRTUAL		EQU	$800	; Not supported

WF_SYS_ALLOC EQU $0001
WF_STEAL_MEM EQU $0002
WF_DONT_REALLOC EQU $0004
WF_ACTIVE EQU $0080
WF_DUP_ON EQU $0400
WF_DIRTY EQU $2000
WF_TRY_SAVE_SCR EQU $4010
WF_VISIBLE EQU $8000

**********************************************************
MAX_TASKID		EQU	1	; ??

CM_IDLE		EQU	$700
CM_INIT		EQU	$701
CM_STARTTASK	EQU	$702
CM_ACTIVATE	EQU	$703
CM_FOCUS	EQU	$704
CM_UNFOCUS	EQU	$705
CM_DEACTIVATE	EQU	$706
CM_ENDTASK	EQU	$707
CM_START_CURRENT	EQU	$708
CM_KEYPRESS	EQU	$710
CM_MENU_CUT	EQU	$720
CM_MENU_COPY	EQU	$721
CM_MENU_PASTE	EQU	$722
CM_STRING	EQU	$723
CM_HSTRING	EQU	$724
CM_DEL		EQU	$725
CM_CLEAR	EQU	$726
CM_MENU_CLEAR	EQU	$727
CM_MENU_FIND	EQU	$728
CM_INSERT	EQU	$730
CM_BLINK	EQU	$740
CM_STORE	EQU	$750
CM_RECALL	EQU	$751
CM_WPAINT	EQU	$760
CM_MENU_OPEN	EQU	$770
CM_MENU_SAVE_AS	EQU	$771
CM_MENU_NEW	EQU	$772
CM_MENU_FORMAT	EQU	$773
CM_MENU_ABOUT	EQU	$774
CM_MODE_CHANGE	EQU	$780
CM_SWITCH_GRAPH	EQU	$781
CM_GEOMETRY	EQU	$7C0

**********************************************************
FLOAT.sign	EQU	0
FLOAT.exponent	EQU	2 	; $2000 < expo < $6000
FLOAT.mantissa	EQU	4
FLOAT.sizeof	EQU	12

BCD.exponent	EQU	0
BCD.mantissa	EQU	2

**********************************************************
SHELL_MAX_LINE			EQU	40
SHELL_HISTORY			EQU	10
SHELL_MAX_PATH			EQU	30

SHELL_AUTO_CHAR			EQU	';'
SCRIPT_COMMENT_CHAR		EQU	'#'
SCRIPT_VARIABLE_CHAR		EQU	'$'
SCRIPT_RETURN_CHAR		EQU	13
SCRIPT_OPEN_BLOCK_CHAR		EQU	'<'
SCRIPT_CLOSE_BLOCK_CHAR		EQU	'>'

SHELL_AUTO_ARCHIVE_FILE_FLAG	EQU	0

PROGRAM_MAX_ATEXIT_FUNC		EQU	10

**********************************************************
DialogSigna			EQU	0	; 4
DialogCallBack			EQU	4	; void *
DialogMenu			EQU	8	; HANDLE
DialogButton			EQU	10	; BYTE / BYTE
DialogSize			EQU	12	; Start of Dialog Struct
DialogSignature			EQU	'DiAl'

POPUP_WIDTH			EQU	98
POPUP_HEIGHT			EQU	50
PMENU_HEIGHT			EQU	70

**********************************************************
ER_THROW	MACRO
	dc.w	$A000+\1
		ENDM
ER_throw	MACRO
	dc.w	$A000+\1
		ENDM

ARG_ERROR			EQU	40
ARG_NAME_ERROR			EQU	140
BREAK_ERROR			EQU	180
CIRCULAR_DEFINITION_ERROR	EQU	190
DUPLICATE_ERROR			EQU	270
FOLDER_ERROR			EQU	330
INVALID_COMMAND_ERROR		EQU	410
INVALID_VARIABLE_ERROR		EQU	620
LINK_TRANSMISSION_ERROR		EQU	650
MEMORY_ERROR			EQU	670
NON_REAL_RESULT_ERROR		EQU	800
SYNTAX_ERROR			EQU	910
TOO_FEW_ARGS_ERROR		EQU	930
TOO_MANY_ARGS_ERROR		EQU	940
UNDEFINED_VARIABLE_ERROR	EQU	960
VARIABLE_IN_USE_ERROR		EQU	970
VARIABLE_IS_8_LIMITED_ERROR	EQU	990

NO_ERROR			EQU	999

; Miss 980

**************************************************************
TIB_EXEC		EQU	$10000			; Where to copy the TIB install code in RAM
TIB_SMALL		EQU	$20000			; First used buffer in RAM by TIB installer
TIB_LARGE		EQU	$24004			; Second used buffer in RAM by TIB Installer
