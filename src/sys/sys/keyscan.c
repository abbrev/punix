#include <string.h>
#include <strings.h> /* for ffs() */
#include <ctype.h>

#include "punix.h"
#include "buf.h"
#include "dev.h"
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
	{ KEY_BACK,KEY_THETA,'l','o','+',KEY_MODE,'\e',KEY_VOID, },
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
 { 'g','`' },
	{ 'h','&' },
#if 0
	{ 'j',0xbe }, /* infinity */
#endif
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
	{ '7',0xbd }, /* integral (1/2) */
	{ '8',0xbc }, /* derivative (1/4) */
	{ '9',0xb4 }, /* ^-1 (accent) */
	{ '*',0xa8 }, /* square root (") */
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
	{ KEY_SIGN,'~' },
#if 0
	{ KEY_SIGN,KEY_ANS },
#endif
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

struct expand {
	short oldkey;
	char expansion[6];
};

static const struct expand expand_table[] = {
	{ KEY_LEFT, "\e[D" },
	{ KEY_UP, "\e[A" },
	{ KEY_RIGHT, "\e[C" },
	{ KEY_DOWN, "\e[B" },
	
	{ KEY_COS, "cos " },
	{ KEY_SIN, "sin " },
	{ KEY_TAN, "tan " },
	{ KEY_LN, "ln " },
/* FIXME: add more */	
#if 1
	{ KEY_INS, "\e[2~" },
	{ KEY_DEL, "\e[3~" },
	{ KEY_F1, "\eOP" },
	{ KEY_F2, "\eOQ" },
	{ KEY_F3, "\eOR" },
	{ KEY_F4, "\eOS" },
	{ KEY_F5, "\e[15~" },
	{ KEY_F6, "\e[17~" },
	{ KEY_F7, "\e[18~" },
	{ KEY_F8, "\e[19~" },
#endif
 { 0, "" }
};

static void expand(short key)
{
	const struct expand *ep = expand_table;
	while (ep->oldkey) {
		if (ep->oldkey == key) {
			char *cp = ep->expansion;
			while (*cp != '\0') {
				G.vt.key = *cp++;
				vtrint(DEV_VT);
			}
			return;
		}
		++ep;
	}
}

struct compose {
	char digraph[2];
	short newkey;
};

static const struct compose compose_table[] = {
	{ "  ", 160 },
	{ "!!", 161 },
	{ "c/", 162 },
	{ "c|", 162 },
	{ "L-", 163 },
	{ "L=", 163 },
	{ "ox", 164 },
	{ "xo", 164 },
	{ "Y-", 165 },
	{ "Y=", 165 },
	{ "||", 166 },
	{ "so", 167 },
	{ "\"\"", 168 },
	{ "(c", 169 },
	{ "a_", 170 },
	{ "<<", 171 },
	{ "-,", 172 },
	{ "--", 173 },
	{ "(r", 174 },
	{ "__", 175 },
	{ "^0", 176 },
	{ "+-", 177 },
	{ "^2", 178 },
	{ "^3", 179 },
	{ "''", 180 },
	{ "u/", 181 },
	{ "P!", 182 },
	{ "p!", 182 },
	{ "!P", 182 },
	{ "!p", 182 },
	{ "..", 183 },
	{ ",,", 184 },
	{ "^1", 185 },
	{ "o_", 186 },
	{ ">>", 187 },
	{ "14", 188 },
	{ "12", 189 },
	{ "34", 190 },
	{ "??", 191 },
	{ "A`", 192 },
	{ "A'", 193 },
	{ "A^", 194 },
	{ "A~", 195 },
	{ "A\"", 196 },
	{ "A*", 197 },
	{ "AE", 198 },
	{ "C,", 199 },
	{ "E`", 200 },
	{ "E'", 201 },
	{ "E^", 202 },
	{ "E\"", 203 },
	{ "I`", 204 },
	{ "I'", 205 },
	{ "I^", 206 },
	{ "I\"", 207 },
	{ "-D", 208 },
	{ "N~", 209 },
	{ "O`", 210 },
	{ "O'", 211 },
	{ "O^", 212 },
	{ "O~", 213 },
	{ "O\"", 214 },
	{ "xx", 215 },
	{ "O/", 216 },
	{ "U`", 217 },
	{ "U'", 218 },
	{ "U^", 219 },
	{ "U\"", 220 },
	{ "Y'", 221 },
	{ "TH", 222 },
	{ "ss", 223 },
	{ "a`", 224 },
	{ "a'", 225 },
	{ "a^", 226 },
	{ "a~", 227 },
	{ "a\"", 228 },
	{ "a*", 229 },
	{ "ae", 230 },
	{ "c,", 231 },
	{ "e`", 232 },
	{ "e'", 233 },
	{ "e^", 234 },
	{ "e\"", 235 },
	{ "i`", 236 },
	{ "i'", 237 },
	{ "i^", 238 },
	{ "i\"", 239 },
	{ "-d", 240 },
	{ "n~", 241 },
	{ "o`", 242 },
	{ "o'", 243 },
	{ "o^", 244 },
	{ "o~", 245 },
	{ "o\"", 246 },
	{ "-:", 247 },
	{ ":-", 247 },
	{ "o/", 248 },
	{ "u`", 249 },
	{ "u'", 250 },
	{ "u^", 251 },
	{ "u\"", 252 },
	{ "y'", 253 },
	{ "th", 254 },
	{ "y\"", 255 },
	/* FIXME: add more */
	{ "", 0 },
};

static short compose(short key)
{
	const struct compose *cp = compose_table;
	while (cp->digraph[0] != '\0') {
		if (cp->digraph[0] == G.vt.key_compose && cp->digraph[1] == key) {
			return cp->newkey;
		}
		++cp;
	}
	return key;
}

/* The status bitmaps and drawmod()/showmods() are kind of a hack. It would be
 * nice if they were rewritten (but not essential since it works as it is) */
static const unsigned short status[][4] = {
#include "glyphsets/status-none.inc"
#include "glyphsets/status-2nd.inc"
#include "glyphsets/status-diamond.inc"
#include "glyphsets/status-shift.inc"
#ifdef TI92P
#include "glyphsets/status-hand.inc"
#else
#include "glyphsets/status-alpha.inc"
#endif
#include "glyphsets/status-capslock.inc"
#include "glyphsets/status-compose1.inc"
#include "glyphsets/status-compose2.inc"
#include "glyphsets/status-bell.inc"
};

#define STATUS_NONE     0
#define STATUS_2ND      1
#define STATUS_DIAMOND  2
#define STATUS_SHIFT    3
#ifdef TI92P
#define STATUS_HAND     4
#else
#define STATUS_ALPHA    4
#endif
#define STATUS_CAPSLOCK 5
#define STATUS_COMPOSE1 6
#define STATUS_COMPOSE2 7
#define STATUS_BELL     8
#define STATUS_LAST     8

static void drawmod(int pos, int m)
{
	/* draw mod 'm' at position 'pos' */
	char *d;
	char *s;
	int i;
	if (m < 0 || STATUS_LAST < m) return;
	if (pos < 0 || 30 <= pos) return;
	//kprintf("drawmod(%d, %d)\n", pos, m);
	s = (char *)(0x4c00L + 0xf00 - 7*30 - 1 - pos);
	d = (char *)&status[m][0];
	for (i = 0; i < 8; ++i) {
		*s = *d;
		s+=30;
		++d;
	}
}

static void showmods(void)
{
	/* TI-92+: */
	/* compose hand capslock shift diamond 2nd bell */
	/* 6       5    4        3     2       1   0    */
	
	int mod = G.vt.key_mod | G.vt.key_mod_sticky;
	
	drawmod(1, mod & KEY_2ND ? STATUS_2ND : STATUS_NONE);
	drawmod(2, mod & KEY_DIAMOND ? STATUS_DIAMOND : STATUS_NONE);
	drawmod(3, mod & KEY_SHIFT ? STATUS_SHIFT : STATUS_NONE);
	drawmod(4, G.vt.key_caps ? STATUS_CAPSLOCK : STATUS_NONE);
	drawmod(5, mod & KEY_HAND ? STATUS_HAND : STATUS_NONE);
	drawmod(6, G.vt.compose ? STATUS_COMPOSE1 : G.vt.key_compose ? STATUS_COMPOSE2 : STATUS_NONE);
}

static void addkey(unsigned short key)
{
	unsigned short mod;
	
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
		else   key |= mod;
	}
	
	if (!!(mod & KEY_SHIFT) ^ G.vt.key_caps) {
#if 0
		key = toupper(key);
#else
		if ('a' <= key && key <= 'z')
			key += 'A' - 'a';
#endif
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
		} else if (key == '2') {
			key = 0;
		} else if ('3' <= key && key <= '7') {
			key -= 0x18;
		} else if (key == '8') {
			key = 0x7f;
		} else if (key == '/') {
			key = 0x1f;
		} else if (key == KEY_BACK) {
			key = KEY_DEL;
		} else if (key == '=') {
			key = '%'; /* XXX */
		} else if (('A' <= key && key <= 'Z')
		           || (0x5b <= key && key <= 0x5f)) {
			key -= 0x40;
		}
	}
	
	if (key == KEY_COMPOSE) {
		if (G.vt.compose)
			G.vt.compose = G.vt.key_compose = 0;
		else
			G.vt.compose = 1;
		goto end;
	}
	
	if (key < 0x100) {
		if (G.vt.compose) {
			G.vt.key_compose = key;
			G.vt.compose = 0;
			goto end;
		} else if (G.vt.key_compose) {
			key = compose(key);
			G.vt.key_compose = 0;
		}
		G.vt.key = key;
		vtrint(DEV_VT);
	} else {
		expand(key);
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
	short rowmask = 1, colmask = 1, key;
	unsigned char *currow = &G.vt.key_array[0];
	int row, col = 0;
	short k;
	unsigned char kdiff;
	int kp;
	KBROWMASK = 0;
	_WaitKeyboard();
	k = KBCOLMASK;
	kp = ~k; /* is a key pressed? */
	for (row = KEY_NBR_ROW - 1; row >= 0; --row, rowmask <<= 1, ++currow) {
		if (kp) {
			KBROWMASK = ~rowmask;
			_WaitKeyboard();
			k = KBCOLMASK;
		}
		kdiff = k ^ *currow;
		*currow = k;
		if (!kdiff) continue;
		
		do {
			int c = ffs(kdiff) - 1;
			col += c;
			colmask <<= c;
			kdiff >>= c;
			kdiff &= ~1;
			key = Translate_Key_Table[KEY_NBR_ROW-row-1][col];
			if (!(*currow & colmask)) {
				/* key was pressed */
				if (key < 0x1000) {
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
			showmods();
		} while (kdiff);
		goto end;
	}
	/* no key was pressed or released, so repeat the previous key */
	if (!G.vt.key_repeat || G.vt.key_previous == 0) goto end;
	if (!--G.vt.key_repeat_counter) {
		G.vt.key_repeat_counter = G.vt.key_repeat_delay;
		addkey(G.vt.key_previous);
	}
end:
	KBROWMASK = 0x380; /* reset to standard key reading */
	return;
}
