#include <string.h>
#include <strings.h> /* for ffs() */
#include <ctype.h>

#include "punix.h"
#include "kbd.h"
#include "lcd.h"
#include "globals.h"

#define KBROWMASK (*(volatile short *)0x600018)
#define KBCOLMASK (*(volatile char *)0x60001b)

static const short Translate_Key_Table[][8] = {
#ifdef TI89
	{ KEY_UP,KEY_LEFT,KEY_DOWN,KEY_RIGHT,KEY_2ND,KEY_SHIFT,KEY_DIAMOND,KEY_ALPHA, },
	{ KEY_ENTER,'+','-','*','/','^',KEY_CLEAR,KEY_F5, },
	{ KEY_SIGN,'3','6','9',',','t',KEY_BACK,KEY_F4, },
	{ '.','2','5','8',')','z',KEY_CATALOG,KEY_F3, },
	{ '0','1','4','7','(','y',KEY_MODE,KEY_F2, },
	{ KEY_APPS,KEY_STO,KEY_EE,KEY_OR,'=','x',KEY_HOME,KEY_F1, },
	{ KEY_ESC,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID, },
#else
	{ KEY_2ND,KEY_DIAMOND,KEY_SHIFT,KEY_HAND,KEY_LEFT,KEY_UP,KEY_RIGHT,KEY_DOWN, },
	{ KEY_VOID,'z','s','w',KEY_F8,'1','2','3', },
	{ KEY_VOID,'x','d','e',KEY_F3,'4','5','6', },
	{ KEY_STO,'c','f','r',KEY_F7,'7','8','9', },
	{ ' ','v','g','t',KEY_F2,'(',')',',', },
	{ '/','b','h','y',KEY_F6,KEY_SIN,KEY_COS,KEY_TAN, },
	{ '^','n','j','u',KEY_F1,KEY_LN,KEY_ENTER,'p', },
	{ '=','m','k','i',KEY_F5,KEY_CLEAR,KEY_APPS,'*', },
	{ KEY_BACK,KEY_THETA,'l','o','+',KEY_MODE,KEY_ESC,KEY_VOID, },
	{ '-',KEY_ENTER,'a','q',KEY_F4,'0','.',KEY_SIGN, },
#endif
};

void _WaitKeyboard(void);

struct translate {
	short oldkey, newkey;
};

static const struct translate Translate_2nd[] = {
	{ 'q','?' },
	{ 'w','!' },
	{ 'e',0xe9 }, /* e' */
	{ 'r','@' },
	{ 't','#' },
	{ 'y',0x1a }, /* solid right triangle */ /* XXX */
	{ 'u',0xfc }, /* u" */
	{ 'i',0x97 }, /* imaginary */
	{ 'o',0xd4 }, /* O^ */
	{ 'p','_' },
	{ 'a',0xe1 }, /* a` */
	{ 's',0x81 }, /* German s XXX */
	{ 'd',0xb0 }, /* degree */
	{ 'f',0x9f }, /* angle XXX */
	{ 'g',0x80 }, /* alpha */ /* XXX */
	{ 'h','&' },
	{ 'j',0xbe }, /* infinity */
	{ 'k','|' },
	{ 'l','"' },
	{ 'x',0xa9 }, /* copyright */
	{ 'c',0xc7 }, /* C, */
	{ 'v',0x9d }, /* not equal XXX */
	{ 'b','\'' },
	{ 'n',0xf1 }, /* n~ */
	{ 'm',';' },
	{ '=','\\' },
	{ KEY_THETA,':' },
	{ '(','{' },
	{ ')','}' },
	{ ',','[' },
	{ '/',']' },
	{ '^',0x8c }, /* pi XXX */
	{ '7',0xbd }, /* integral (1/2) */
	{ '8',0xbc }, /* derivative (1/4) */
	{ '9',0xb4 }, /* ^-1 (accent) */
	{ '*',0xa8 }, /* square root (") */
	{ '4',0x8e }, /* Sigma XXX */
	{ '5',KEY_MATH },
	{ '6',KEY_MEM },
	{ '-',KEY_VARLINK },
	{ '1',KEY_EE },
	{ '2',KEY_CATALOG },
	{ '3',KEY_CUSTOM },
	{ '+',KEY_CHAR },
	{ '0','<' },
	{ '.','>' },
	{ KEY_SIGN,KEY_ANS },
	{ KEY_BACK,KEY_INS },
	{ KEY_ENTER,KEY_ENTRY },
	{ KEY_APPS,KEY_SWITCH },
	{ KEY_ESC,KEY_QUIT },
	{ KEY_STO,KEY_RCL },
	{ ' ','$' },
	{ 0, 0 }
};

static short translate(short key, const struct translate *tp)
{
	while (tp->oldkey) {
		if (tp->oldkey == key) {
			return tp->newkey;
		}
	}
	return 0;
}

void addkey(short key)
{
	if (key >= 0x1000) {
		/* status key */
		G.vt.key_status ^= key; /* toggle this status key */
		return;
	}
	
	/* normal key */
	if (G.vt.key_status & KEY_2ND) {
		short k;
		if (key == 'z') {
			/* caps lock */
			G.vt.key_caps = !G.vt.key_caps;
			goto end;
		}
		k = translate(key, Translate_2nd);
		if (k) key = k;
		/*else   key |= key_status;*/
	}
	if (G.vt.key_status == KEY_DIAMOND) {
		if (key == '+') {
			lcd_inc_contrast();
			goto end;
		} else if (key == '-') {
			lcd_dec_contrast();
			goto end;
		}
		if ('a' <= key && key <= 'z') {
			key -= 0x40;
		} else if ('3' <= key && '7' <= key) {
			key -= 0x18;
		} else if (key == '8') {
			key = 0x7f;
		} else if (key == '/') {
			key = 0x1f;
		}
	}
	
	if ((G.vt.key_status & KEY_SHIFT) ^ G.vt.key_caps) {
#if 0
		key = toupper(key);
#else
		if ('a' <= key && key <= 'z')
			key += 'A' - 'a';
#endif
	}
	
	if (key < 0x100) {
		G.vt.key = key;
		vtrint(0x0100);
	}
end:
	G.vt.key_status = 0;
	G.vt.key_mask[0] &= RESET_KEY_STATUS_MASK;
#if 0
	G.vt.key_auto_status = 1;
#endif
}

void scankb()
{
	int row, col;
	short key_pressed = 0;
	static short key_previous, key_cur_row, key_cur_col;
	char *curmask;
	short rowmask, j, k;
	
loop:
	curmask = &G.vt.key_mask[0];
	/* check if a key is pressed */
	KBROWMASK = 0; /* read all keys */
	_WaitKeyboard();

	if (~KBCOLMASK) {
		/* A key is pressed. Check which key is pressed */
		key_pressed = 0;
		rowmask = 1;
		for (row = KEY_NBR_ROW - 1; row >= 0; --row, rowmask <<= 1) {
			KBROWMASK = ~rowmask; /* select row */
			_WaitKeyboard();
			k = KBCOLMASK; /* read which keys are pressed */
			j = k; /* add the key mask */
			k = k | *curmask; /* clear some keys according to the mask */
			*curmask++ &= ~j; /* update the mask */
			if (!~k) continue; /* no key is pressed */
			
			/* find out which key is pressed */
			col = ffs(k) - 1;
			key_cur_row = rowmask; /* remember which key is currently pressed */
			key_cur_col = 1 << col;
			curmask[-1] |= key_cur_col; /* update mask so that this key won't be added again */
			
			key_pressed = Translate_Key_Table[row][col];
			if (key_pressed >= 0x1000)
				goto end;
			
			G.vt.key_prev_row = key_cur_row;
			G.vt.key_prev_col = key_cur_col;
			G.vt.key_previous = key_pressed;
			G.vt.key_counter = G.vt.key_repeat_start_delay; /* start delay before repeating it */
			break;
		}
		
		/* no new keys were pressed - auto repeat previous key */
		if (key_previous == 0 || key_pressed >= 0x1000) {
			key_pressed = 0;
			goto end;
		}
		/* check if previous key is still pressed. */
		KBROWMASK = ~key_cur_row; /* select row */
		_WaitKeyboard();
		k = KBCOLMASK; /* read which keys are pressed */
		if (k & key_cur_col) { /* previous key is not pressed */
			G.vt.key_mask[0] &= RESET_KEY_STATUS_MASK; /* clear mask for status keys */
			G.vt.key_status = G.vt.key_previous = 0;
			goto loop;
		}
		if (!--G.vt.key_counter) {
			G.vt.key_counter = G.vt.key_repeat_delay;
			key_pressed = G.vt.key_previous /* | KEY_AUTO_REPEAT */;
		} else
			key_pressed = 0;
	} else {
		/* no key is currently being pressed */
		memset(G.vt.key_mask, 0, KEY_NBR_ROW); /* reset key_mask */
		G.vt.key_previous = 0;
#if 0
		if (G.vt.key_auto_status)
			G.vt.key_auto_status = G.vt.key_status = 0;
#endif
	}
end:
	KBROWMASK = 0x380; /* reset to standard key reading */
	if (key_pressed)
		addkey(key_pressed);
}
