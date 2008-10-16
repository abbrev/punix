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
	{ '\t','c','f','r',KEY_F7,'7','8','9', },
	{ ' ','v','g','t',KEY_F2,'(',')',',', },
	{ '/','b','h','y',KEY_F6,KEY_SIN,KEY_COS,KEY_TAN, },
	{ '^','n','j','u',KEY_F1,KEY_LN,KEY_ENTER,'p', },
	{ '=','m','k','i',KEY_F5,KEY_CLEAR,KEY_APPS,'*', },
	{ '\b',KEY_THETA,'l','o','+',KEY_MODE,'\x1b',KEY_VOID, },
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
#if 0
	{ 'y',0x1a }, /* solid right triangle */ /* XXX */
#endif
	{ 'u',0xfc }, /* u" */
	{ 'i',0x97 }, /* imaginary */
	{ 'o',0xd4 }, /* O^ */
	{ 'p','_' },
	{ 'a',0xe1 }, /* a` */
#if 0
	{ 's',0x81 }, /* German s XXX */
#endif
	{ 'd',0xb0 }, /* degree */
#if 0
	{ 'f',0x9f }, /* angle XXX */
	{ 'g',0x80 }, /* alpha */ /* XXX */
#endif
	{ 'h','&' },
	{ 'j',0xbe }, /* infinity */
	{ 'k','|' },
	{ 'l','"' },
	{ 'x',0xa9 }, /* copyright */
	{ 'c',0xc7 }, /* C, */
#if 0
	{ 'v',0x9d }, /* not equal XXX */
#endif
	{ 'b','\'' },
	{ 'n',0xf1 }, /* n~ */
	{ 'm',';' },
	{ '=','\\' },
	{ KEY_THETA,':' },
	{ '(','{' },
	{ ')','}' },
	{ ',','[' },
	{ '/',']' },
#if 0
	{ '^',0x8c }, /* pi XXX */
#endif
	{ '7',0xbd }, /* integral (1/2) */
	{ '8',0xbc }, /* derivative (1/4) */
	{ '9',0xb4 }, /* ^-1 (accent) */
	{ '*',0xa8 }, /* square root (") */
#if 0
	{ '4',0x8e }, /* Sigma XXX */
#endif
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
		if (tp->oldkey == key)
			return tp->newkey;
		++tp;
	}
	return 0;
}

void addkey(short key)
{
	short mod;
	if (key >= 0x1000) {
		G.vt.key_mod_sticky ^= key;
		return;
	}
	
	mod = G.vt.key_mod | G.vt.key_mod_sticky;
	
	if (mod & KEY_2ND) {
		short k;
		if (key == 'z') {
			/* caps lock */
			G.vt.key_caps = !G.vt.key_caps;
			goto end;
		}
		k = translate(key, Translate_2nd);
		if (k) key = k;
		/*else   key |= key_mod;*/
	}
	if (mod & KEY_DIAMOND) {
		if (key == '+') {
			lcd_inc_contrast();
			goto end;
		} else if (key == '-') {
			lcd_dec_contrast();
			goto end;
		}
		if ('a' <= key && key <= 'z') {
			key -= 0x60;
		} else if ('3' <= key && key <= '7') {
			key -= 0x18;
		} else if (key == '8') {
			key = 0x7f;
		} else if (key == '/') {
			key = 0x1f;
		}
	}
	
	if (!!(mod & KEY_SHIFT) ^ G.vt.key_caps) {
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
	G.vt.key_mod_sticky = 0;
	/*G.vt.key_array[0] &= RESET_KEY_STATUS_MASK;*/
#if 0
	G.vt.key_auto_status = 1;
#endif
}

void scankb()
{
	short rowmask = 1, colmask, key;
	unsigned char *currow = &G.vt.key_array[0];
	int row, col;
	short k;
	unsigned char kdiff;
	for (row = KEY_NBR_ROW - 1; row >= 0; --row, rowmask <<= 1, ++currow) {
		KBROWMASK = ~rowmask;
		_WaitKeyboard();
		k = KBCOLMASK;
		kdiff = k ^ *currow;
		*currow = k;
		if (!kdiff) continue;
		
		while (kdiff) {
			col = ffs(kdiff) - 1;
			colmask = 1 << col;
			kdiff &= ~colmask;
			key = Translate_Key_Table[KEY_NBR_ROW-row-1][col];
			if (!(*currow & colmask)) {
				/* key was pressed */
				if (key < 0x1000) {
					G.vt.key_row_mask = rowmask;
					G.vt.key_col_mask = colmask;
					G.vt.key_previous = key;
					G.vt.key_repeat_counter = G.vt.key_repeat_start_delay;
				} else {
					G.vt.key_mod |= key;
				}
				addkey(key);
			} else {
				/* key was released */
				if (key < 0x1000) {
					if (key == G.vt.key_previous) {
						G.vt.key_previous = 0;
					}
				} else {
					G.vt.key_mod &= ~key;
				}
			}
		}
		goto end;
	}
	/* no key was pressed or released, so repeat the previous key */
	if (G.vt.key_previous == 0) goto end;
	if (!--G.vt.key_repeat_counter) {
		G.vt.key_repeat_counter = G.vt.key_repeat_delay;
		addkey(G.vt.key_previous);
	}
end:
	KBROWMASK = 0x380; /* reset to standard key reading */
	return;
}
