#ifndef _KBD_H_
#define _KBD_H_

#if CALC_HAS_QWERTY_KBD
#define KEY_NBR_ROW 10
#define HELPKEYS_KEY (KEY_DIAMOND|'k')
#else
#define KEY_NBR_ROW 7
#define HELPKEYS_KEY (KEY_DIAMOND|KEY_EE)
#endif


#if 0
/* these keycodes are the same as ASCII */
#define KEY_VOID	 0
#define KEY_ENTER	 13	/* carriage return (not linefeed) */
#define KEY_STO		 22	/* this is tab in Punix */
#define KEY_ESC		 27
#define KEY_OR		 124	/* pipe | */
#define KEY_BACK	 127	/* DEL */
#endif

enum {
	KEY_SIGN = 173,
	/* keycodes below are not in ASCII. 0x100 <= keycode < 0x1000 */
	KEY_THETA = 256,
	KEY_LN,
	KEY_SIN,
	KEY_COS,
	KEY_TAN,
	KEY_ETOX,
	KEY_ASIN,
	KEY_ACOS,
	KEY_ATAN,
	KEY_XTONEGONE,
	KEY_CLEAR,
	KEY_APPS,
	KEY_MODE,
	KEY_COMPOSE = KEY_MODE,
	KEY_ON,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_DEL,
	KEY_CATALOG,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_PGUP,
	KEY_PGDOWN,

	KEY_INS,
	KEY_OFF,
	KEY_QUIT,
	KEY_SWITCH,
	KEY_VARLINK,
	KEY_CHAR,
	KEY_ENTRY,
	KEY_RCL,
	KEY_MATH,
	KEY_MEM,
	KEY_ANS,
	KEY_CUSTOM,
	KEY_UNITS,
	KEY_OFF2,
	KEY_ANGLE,
	KEY_PI,
	KEY_INTEGRAL,
	KEY_DERIVATIVE,
	KEY_SQRT,

#if 0
	/* what *are* these?? */
	KEY_INFEQUAL,
	KEY_DIFERENT,
	KEY_SUPEQUAL,
#endif

	KEY_EE,
	KEY_HOME,
	KEY_END,

	/* not in TI-AMS */
	KEY_CAPSLOCK,
	KEY_ALPHALOCK,
	KEY_HANDLOCK,


	/* modifiers. these are bit fields */
	KEY_2ND	    = 0x1000,
	KEY_DIAMOND = 0x2000,
	KEY_SHIFT   = 0x4000,
	KEY_HAND    = 0x8000,
	KEY_ALPHA   = 0x8000,
};


#endif
