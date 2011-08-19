#include <string.h>
#include <strings.h> /* for ffs() */
#include <ctype.h>

#include "punix.h"
#include "buf.h"
#include "dev.h"
#include "kbd.h"
#include "lcd.h"
#include "globals.h"
#include "cell.h"

#define KBROWMASK (*(volatile short *)0x600018)
#define KBCOLMASK (*(volatile char *)0x60001b)

#define CR  '\r'
#define ESC '\e'
#define TAB '\t'
#define DEL 127

static const short Translate_Key_Table[][8] = {
#ifdef TI89
	{ KEY_UP,KEY_LEFT,KEY_DOWN,KEY_RIGHT,KEY_2ND,KEY_SHIFT,KEY_DIAMOND,KEY_ALPHA, },
	{  CR,'+','-','*','/','^',KEY_CLEAR,KEY_F5, },
	{ KEY_SIGN,'3','6','9',',','t',DEL,KEY_F4, },
	{ '.','2','5','8',')','z',KEY_CATALOG,KEY_F3, },
	{ '0','1','4','7','(','y',KEY_MODE,KEY_F2, },
	{ KEY_APPS,TAB,KEY_EE,'|','=','x',KEY_HOME,KEY_F1, },
	{ ESC,  0,  0,  0,  0,  0,  0,  0, },
#else
	{ KEY_2ND,KEY_DIAMOND,KEY_SHIFT,KEY_HAND,KEY_LEFT,KEY_UP,KEY_RIGHT,KEY_DOWN, },
	{   0,'z','s','w',KEY_F8,'1','2','3', },
	{   0,'x','d','e',KEY_F3,'4','5','6', },
	{ TAB,'c','f','r',KEY_F7,'7','8','9', },
	{ ' ','v','g','t',KEY_F2,'(',')',',', },
	{ '/','b','h','y',KEY_F6,KEY_SIN,KEY_COS,KEY_TAN, },
	{ '^','n','j','u',KEY_F1,KEY_LN, CR,'p', },
	{ '=','m','k','i',KEY_F5,KEY_CLEAR,KEY_APPS,'*', },
	{ DEL,KEY_THETA,'l','o','+',KEY_MODE,ESC,  0, },
	{ '-', CR,'a','q',KEY_F4,'0','.',KEY_SIGN, },
#endif
};

void _WaitKeyboard(void);

struct translate {
	short oldkey, newkey;
};

static const struct translate Translate_2nd[] = {
	/* common 2nd key translations between TI-89 and TI-92+ */
	{ '(','{' },
	{ ')','}' },
	{ ',','[' },
	{ '/',']' },
	{ '0','<' },
	{ '.','>' },
	{ DEL,KEY_INS },
	{ CR,KEY_ENTRY },
	{ ESC,KEY_QUIT },
	{ TAB,KEY_RCL },
#if 0
	/* these characters are not in ISO-8859-1 */
	{ '^',KEY_PI }, /* pi XXX */
	{ '7',KEY_INTEGRAL }, /* integral (1/2) */
	{ '8',KEY_DERIVATIVE }, /* derivative (1/4) */
	{ '*',KEY_SQRT }, /* square root (") */
#endif
	{ '5',KEY_MATH },
	{ '6',KEY_MEM },
	{ '-',KEY_VARLINK },
	{ '+',KEY_CHAR },
#if 0
	{ KEY_SIGN,KEY_ANS },
#else
	{ KEY_SIGN,'~' },
#endif
	{ KEY_UP, KEY_PGUP },
	{ KEY_DOWN, KEY_PGDOWN },

#ifdef TI92P
	{ 'q','?' },
	{ 'w','!' },
	{ 'e',0xe9 }, /* e' */
	{ 'r','@' },
	{ 't','#' },
	{ 'u',0xfc }, /* u" */
	{ 'o',0xd4 }, /* O^ */
	{ 'p','_' },
	{ 'a',0xe1 }, /* a` */
	{ 's',0xdf }, /* German double s */
	{ 'd',0xb0 }, /* degree */
	{ 'g','`' },
	{ 'h','&' },
	{ 'k','|' },
	{ 'l','"' },
	{ 'z',KEY_CAPSLOCK },
	{ 'x',0xa9 }, /* copyright */
	{ 'c',0xc7 }, /* C, */
	{ 'b','\'' },
	{ 'n',0xf1 }, /* n~ */
	{ 'm',';' },
	{ '=','\\' },
	{ KEY_THETA,':' },
	{ '1',KEY_EE },
	{ '2',KEY_CATALOG },
	{ '3',KEY_CUSTOM },
	{ KEY_APPS,KEY_SWITCH },
	{ ' ','$' },
	{ KEY_HAND, KEY_HANDLOCK },
	{ KEY_LN, KEY_ETOX },
	{ KEY_SIN, KEY_ASIN },
	{ KEY_COS, KEY_ACOS },
	{ KEY_TAN, KEY_ATAN },
	{ '9', KEY_XTONEGONE },
#if 0
	/* these characters are not in ISO-8859-1 */
	{ 'y',0x1a }, /* solid right triangle */ /* XXX */
	{ 'i',0x97 }, /* imaginary */
	{ 'f',KEY_ANGLE }, /* angle XXX */
	{ 'g',0x80 }, /* alpha */ /* XXX */
	{ 'j',0xbe }, /* infinity */
	{ 'v',0x9d }, /* not equal XXX */
	{ '9',0xb4 }, /* ^-1 (accent) */
	{ '4',0x8e }, /* Sigma XXX */
#endif
#else /* TI89 */
	{ KEY_F1, KEY_F6 },
	{ KEY_F2, KEY_F7 },
	{ KEY_F3, KEY_F8 },
	{ 'x', KEY_LN },
	{ 'y', KEY_SIN },
	{ 'z', KEY_COS },
	{ 't', KEY_TAN },
	{ '=', '\'' },
	{ '|', 0xb0 }, /* degree */
	{ '9', ';' },
	{ '4', ':' },
	{ '1', '"' },
	{ '2', '\\' },
	{ '3', KEY_UNITS },
	{ KEY_HOME, KEY_CUSTOM },
	{ KEY_ALPHA, KEY_ALPHALOCK },
	{ KEY_SHIFT, KEY_CAPSLOCK },
#if 0
	{ KEY_EE, KEY_ANGLE },
	{ KEY_MODE, KEY_ARROW }, /* solid right arrow */
#endif
#endif
	{ 0, 0 }
};

#ifdef TI89
static const struct translate Translate_alpha[] = {
	{ '=', 'a' },
	{ '(', 'b' },
	{ ')', 'c' },
	{ ',', 'd' },
	{ '/', 'e' },
	{ '|', 'f' },
	{ '7', 'g' },
	{ '8', 'h' },
	{ '9', 'i' },
	{ '*', 'j' },
	{ KEY_EE, 'k' },
	{ '4', 'l' },
	{ '5', 'm' },
	{ '6', 'n' },
	{ '-', 'o' },
	{ TAB, 'p' },
	{ '1', 'q' },
	{ '2', 'r' },
	{ '3', 's' },
	{ '+', 'u' },
	{ '0', 'v' },
	{ '.', 'w' },
	{ KEY_SIGN, ' ' },
	{ 0, 0 },
};
#else
static const struct translate Translate_hand[] = {
	{ 0, 0 },
};
#endif

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
	{ KEY_PGUP, "\e[5~" },
	{ KEY_PGDOWN, "\e[6~" },
	{ KEY_HOME, "\eOH" },
	{ KEY_END, "\eOF" },
	
	{ KEY_COS, "cos(" },
	{ KEY_SIN, "sin(" },
	{ KEY_TAN, "tan(" },
	{ KEY_LN, "ln(" },
	{ KEY_ACOS, "acos(" },
	{ KEY_ASIN, "asin(" },
	{ KEY_ATAN, "atan(" },
	{ KEY_ETOX, "e^(" },
	{ KEY_XTONEGONE, "^-1" },
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
	{ KEY_CLEAR, "clear" },
	{ 0, "" }
};

static void expand(short key)
{
	const struct expand *ep = expand_table;
	while (ep->oldkey) {
		if (ep->oldkey == key) {
			const char *cp = ep->expansion;
			while (*cp != '\0')
				vtrint(DEV_VT, *cp++);
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

/* The status bitmaps and drawmod()/showstatus() are kind of a hack. It would be
 * nice if they were rewritten (but not essential since it works as it is) */
#ifdef TI92P
#define STATUSROWS 8
static const unsigned short status[][4] = {
#include "glyphsets/status-none.inc"
#include "glyphsets/status-2nd.inc"
#include "glyphsets/status-diamond.inc"
#include "glyphsets/status-shift.inc"
#include "glyphsets/status-hand.inc"
#include "glyphsets/status-handlock.inc"
#include "glyphsets/status-capslock.inc"
#include "glyphsets/status-compose1.inc"
#include "glyphsets/status-compose2.inc"
#include "glyphsets/status-bell.inc"
#include "glyphsets/status-scrolllock.inc"
#include "glyphsets/status-batt0.inc"
#include "glyphsets/status-batt1.inc"
#include "glyphsets/status-batt2.inc"
#include "glyphsets/status-batt3.inc"
#include "glyphsets/status-batt4.inc"
#include "glyphsets/status-busy.inc"
};
#else
#define STATUSROWS 4
static const unsigned short status[][2] = {
#include "glyphsets/status-none-89.inc"
#include "glyphsets/status-2nd-89.inc"
#include "glyphsets/status-diamond-89.inc"
#include "glyphsets/status-shift-89.inc"
#include "glyphsets/status-alpha-89.inc"
#include "glyphsets/status-alphalock-89.inc"
#include "glyphsets/status-capslock-89.inc"
#include "glyphsets/status-compose1-89.inc"
#include "glyphsets/status-compose2-89.inc"
#include "glyphsets/status-bell-89.inc"
#include "glyphsets/status-scrolllock-89.inc"
#include "glyphsets/status-batt0-89.inc"
#include "glyphsets/status-batt1-89.inc"
#include "glyphsets/status-batt2-89.inc"
#include "glyphsets/status-batt3-89.inc"
#include "glyphsets/status-batt4-89.inc"
#include "glyphsets/status-busy-89.inc"
};
#endif

#define STATUS_NONE        0
#define STATUS_2ND         1
#define STATUS_DIAMOND     2
#define STATUS_SHIFT       3
#ifdef TI92P
#define STATUS_HAND        4
#define STATUS_HANDLOCK    5
#else
#define STATUS_ALPHA       4
#define STATUS_ALPHALOCK   5
#endif
#define STATUS_CAPSLOCK    6
#define STATUS_COMPOSE1    7
#define STATUS_COMPOSE2    8
#define STATUS_BELL        9
#define STATUS_SCROLLLOCK 10
#define STATUS_BATT0      11
#define STATUS_BATT1      12
#define STATUS_BATT2      13
#define STATUS_BATT3      14
#define STATUS_BATT4      15
#define STATUS_BUSY       16
#define STATUS_LAST       16

#ifdef TI92P
#define MODBASE ((char *)(LCD_MEM+0xf00-7*30-1))
#else
#define MODBASE ((char *)(LCD_MEM+6*NUMCELLROWS*LCD_INCY+19))
#endif

static void drawmod(int pos, int m)
{
	/* draw mod 'm' at position 'pos' */
	char *d;
	char *s;
	int i;
	if (m < 0 || STATUS_LAST < m) return;
	if (pos < 0 || 30 <= pos) return;
	//kprintf("drawmod(%d, %d)\n", pos, m);
	s = MODBASE - pos;
	d = (char *)&status[m][0];
	for (i = 0; i < STATUSROWS; ++i) {
		*s = *d;
		s+=30;
		++d;
	}
}

void showstatus(void)
{
	/* TI-92+: */
	/* scroll-lock compose hand capslock shift diamond 2nd busy bell */
	/* 8           7       6    5        4     3       2   1    0    */
	
	int batt;
	//int x = spl7();
	mask(&G.calloutlock);
	int mod = G.vt.key_mod | G.vt.key_mod_sticky;
	
	batt = G.batt_level - 3;
	if (batt < 0) batt = 0;
	
	drawmod(0, G.vt.bell ? STATUS_BELL : STATUS_BATT0+batt);
	drawmod(1, G.cpubusy ? STATUS_BUSY : STATUS_NONE);
	drawmod(2, mod & KEY_2ND ? STATUS_2ND : STATUS_NONE);
	drawmod(3, mod & KEY_DIAMOND ? STATUS_DIAMOND : STATUS_NONE);
	drawmod(4, mod & KEY_SHIFT ? STATUS_SHIFT : STATUS_NONE);
	drawmod(5, G.vt.caps_lock ? STATUS_CAPSLOCK : STATUS_NONE);
#ifdef TI92P
	drawmod(6, G.vt.hand_lock ? STATUS_HANDLOCK :
	           mod & KEY_HAND ? STATUS_HAND : STATUS_NONE);
#else
	drawmod(6, G.vt.alpha_lock ? STATUS_ALPHALOCK :
	           mod & KEY_ALPHA ? STATUS_ALPHA : STATUS_NONE);
#endif
	drawmod(7, G.vt.compose ? STATUS_COMPOSE1 : G.vt.key_compose ? STATUS_COMPOSE2 : STATUS_NONE);
	drawmod(8, G.vt.scroll_lock ? STATUS_SCROLLLOCK : STATUS_NONE);
	mask(&G.calloutlock);
	//splx(x);
}

/* callback for timeout() to remove bell status */
void unbell(void *arg)
{
	struct tty *ttyp = (struct tty *)arg;
	G.vt.bell = 0;
	showstatus();
}

/* set the bell status and set a timeout to remove the bell status after
 * some time */
void bell(struct tty *ttyp)
{
	untimeout(unbell, ttyp);
	G.vt.bell = 1;
	showstatus();
	timeout(unbell, ttyp, 1 * HZ); /* XXX: constant */
}

static void addkey(unsigned short key)
{
	unsigned short mod;
	short k;
	
	mod = G.vt.key_mod | G.vt.key_mod_sticky;
	
	/* translate alpha and hand first */
#ifdef TI89
	if (!!(mod & KEY_ALPHA) ^ G.vt.alpha_lock) {
		k = translate(key, Translate_alpha);
		if (k) key = k;
	}
#else
	if (!!(mod & KEY_HAND) ^ G.vt.hand_lock) {
		k = translate(key, Translate_hand);
		if (k) key = k;
	}
#endif
	
	/* translate 2nd (alt) second */
	if (mod & KEY_2ND) {
		k = translate(key, Translate_2nd);
		if (k)
			key = k;
		else if (key < 0x1000) 
			key |= mod;
	}
	
	/* handle alpha/hand lock */
#ifdef TI89
	/* cycle alpha->alpha lock->none */
	if (((mod & KEY_ALPHA) || G.vt.alpha_lock) && key == KEY_ALPHA) {
		G.vt.alpha_lock = !G.vt.alpha_lock;
		G.vt.key_mod_sticky &= ~KEY_ALPHA;
		return;
	}
	
	if (key == KEY_ALPHALOCK) {
		G.vt.alpha_lock = !G.vt.alpha_lock;
		goto end;
	}
#else
	/* cycle hand->hand lock->none */
	if (((mod & KEY_HAND) || G.vt.hand_lock) && key == KEY_HAND) {
		G.vt.hand_lock = !G.vt.hand_lock;
		G.vt.key_mod_sticky &= ~KEY_HAND;
		return;
	}
	
	if (key == KEY_HANDLOCK) {
		G.vt.hand_lock = !G.vt.hand_lock;
		goto end;
	}
#endif
	
	/* handle caps lock */
	if (key == KEY_CAPSLOCK) {
		G.vt.caps_lock = !G.vt.caps_lock;
		goto end;
	}

	if (key >= 0x1000) {
		G.vt.key_mod_sticky ^= key;
		return;
	}
	
	/* handle diamond (control) */
	if (mod & KEY_DIAMOND) {
		if (key == '+') {
			lcd_inc_contrast();
			goto end;
		} else if (key == '-') {
			lcd_dec_contrast();
			goto end;
		}
		
		/* 
		 * this could also be done with a "translate" table, but it
		 * would be large given the wide range of characters to
		 * translate
		 */
		if (0x61 <= key && key <= 0x7e) {
			/* a-z, {, |, }, ~ */
			key -= 0x60;
		} else if (0x5b <= key && key <= 0x5f) {
			/* [, \, ], ^, _ */
			key -= 0x40;
		} else if (key == '2') {
			key = 0;
		} else if ('3' <= key && key <= '7') {
			key -= 0x18;
		} else if (key == '8') {
			key = 0x7f;
		} else if (key == DEL) {
			key = KEY_DEL;
		} else if (key == '=') {
			key = '%'; /* XXX: inequal sign in TI-AMS */
		} else if (key == '0') {
			key = 0xab; /* << XXX: <= in TI-AMS */
		} else if (key == '.') {
			key = 0xbb; /* >> XXX: >= in TI-AMS */
		} else if (key == KEY_UP) {
			key = KEY_HOME;
		} else if (key == KEY_DOWN) {
			key = KEY_END;
#ifdef TI89
		} else if (key == KEY_MODE) {
			key = '_';
		} else if (key == '/') {
			key = '!';
		} else if (key == '*') {
			key = '&';
		} else if (key == ')') {
			key = 0xa9; /* copyright */
#else
		} else if (key == '/') {
			key = 0x1f;
#endif
		}
	}
	
	/* convert lowercase to uppercase */
	if (!!(mod & KEY_SHIFT) ^ G.vt.caps_lock) {
#if 0
		key = toupper(key);
#else
		if ('a' <= key && key <= 'z')
			key += 'A' - 'a';
#endif
	}
	
	if (key == KEY_COMPOSE) {
		if (G.vt.compose || G.vt.key_compose)
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
		vtrint(DEV_VT, key);
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
			showstatus();
		} while (kdiff);
		showstatus();
		goto end;
	}
	/* no key was pressed or released, so repeat the previous key */
	if (G.vt.key_repeat &&
	    G.vt.key_previous != 0 &&
	    --G.vt.key_repeat_counter == 0) {
		G.vt.key_repeat_counter = G.vt.key_repeat_delay;
		addkey(G.vt.key_previous);
	}
end:
	KBROWMASK = 0x380; /* reset to standard key reading */
	return;
}
