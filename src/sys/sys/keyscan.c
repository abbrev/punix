#define KBROWMASK (*(volatile short *)0x600018)
#define KBCOLMASK (*(volatile char *)0x60001b)

static const short Translate_Key_Table[8][] = {
#ifdef TI89
	KEY_UP,KEY_LEFT,KEY_DOWN,KEY_RIGHT,KEY_2ND,KEY_SHIFT,KEY_DIAMOND,KEY_ALPHA,
	KEY_ENTER,'+','-','*','/','^',KEY_CLEAR,KEY_F5,
	KEY_SIGN,'3','6','9',',','t',KEY_BACK,KEY_F4,
	'.','2','5','8',')','z',KEY_CATALOG,KEY_F3,
	'0','1','4','7','(','y',KEY_MODE,KEY_F2,
	KEY_APPS,KEY_STO,KEY_EE,KEY_OR,'=','x',KEY_HOME,KEY_F1,
	KEY_ESC,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,KEY_VOID,
#else
	KEY_2ND,KEY_DIAMOND,KEY_SHIFT,KEY_HAND,KEY_LEFT,KEY_UP,KEY_RIGHT,KEY_DOWN,
	KEY_VOID,'z','s','w',KEY_F8,'1','2','3',
	KEY_VOID,'x','d','e',KEY_F3,'4','5','6',
	KEY_STO,'c','f','r',KEY_F7,'7','8','9',
	' ','v','g','t',KEY_F2,'(',')',',',
	'/','b','h','y',KEY_F6,KEY_SIN,KEY_COS,KEY_TAN,
	'^','n','j','u',KEY_F1,KEY_LN,KEY_ENTER,'p',
	'=','m','k','i',KEY_F5,KEY_CLEAR,KEY_APPS,'*',
	KEY_BACK,KEY_THETA,'l','o','+',KEY_MODE,KEY_ESC,KEY_VOID,
	'-',KEY_ENTER,'a','q',KEY_F4,'0','.',KEY_SIGN,
#endif
};

extern char key_mask[KEY_NBR_ROW];

#define KEY_NBR_ROW 10 /* 10 for 92+, 7 for 89 */
| In: 
|	Nothing
| Out:
|	d4.w = Key
| Destroy:
|	All !
int KeyScan()
{
	int row, col;
	short key_pressed = 0;
	static short key_previous, key_cur_row, key_cur_col;
	char *curmask;
	
loop:
	curmask = &key_mask[0];
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
			int col = ffs(k) - 1;
			key_cur_row = rowmask; /* remember which key is currently pressed */
			key_cur_col = 1 << col;
			curmask[-1] |= key_cur_col; /* update mask so that this key won't be added again */
			
			key_pressed = Translate_Key_Table[row][col];
			key_previous = key_pressed;
			key_counter = key_org_start_counter; /* start delay before repeating it */
			goto end;
		}
		
		/* Auto Repeat Feature */
		if (key_previous == 0 || (key_pressed >= 0x1000)) {
			key_pressed = 0;
			goto end;
		}
		/* check if previous key is still pressed. */
		KBROWMASK = ~key_cur_row; /* select row */
		_WaitKeyboard();
		k = KBCOLMASK; /* read which keys are pressed */
		if (k & key_cur_col) { /* previous key is not pressed */
			key_mask[0] &= RESET_KEY_STATUS_MASK; /* clear mask for status keys */
			key_status = key_previous = 0;
			goto loop;
		}
		if (!--key_counter) {
			key_counter = key_org_repeat_counter;
			key_pressed = key_previous | KEY_AUTO_REPEAT;
		} else
			key_pressed = 0;
	} else {
		memset(key_mask, 0, KEY_NBR_ROW); /* reset key_mask */
		key_previous = 0;
		if (key_auto_status)
			key_auto_status = key_status = 0;
		else
			key_pressed = 0;
	}
end:
	KBROWMASK = 0x380; /* reset to standard key reading */
	return key_pressed;
}

struct translate {
	short oldkey, newkey;
};

static const struct translate Translate_2nd[] = {
	/* XXX */
	{ 0, 0 }
};

static short translate(short key, struct translate *tp)
{
	while (tp->oldkey) {
		if (tp->oldkey == key) {
			return tp->newkey;
		}
	}
	return 0;
}

void AddKey(short key)
{
	const struct translate *tp;
	if (key >= 0x1000) {
		/* status key */
		if (key == key_status)
			key_status &= ~key;
		else
			key_status |= key;
		return;
	}
	
	/* normal key */
	if (key_status & KEY_2ND) {
		short k;
		if (key == 'z') {
			/* caps lock */
			key_caps = !key_caps;
			goto end;
		}
		k = translate(key, Translate_2nd);
		if (k) key = k;
		else   key |= key_status;
	}
	if (key_status == KEY_DIAMOND) {
		if (key == '+') {
			lcd_inc_contrast();
			goto end;
		} else if (key == '-') {
			lcd_dec_contrast();
			goto end;
		}
		/* FIXME: translate Ctrl-key */
		if ('a' <= key && key <= 'z') {
			key -= 0x40;
		} else {
			switch (key) {
			case '[':
			case '\\':
			case ']':
			case '^':
			case '_':
			case '`':
				key -= 0x40;
				break;
			}
		}
	}
	
add_key:
	if ((key_status & KEY_SHIFT) ^ key_caps) {
		key = toupper(key);
	}
	
	AddKeyToFIFOKeyBuffer(key);
end:
	key_status = 0;
	key_mask[0] &= RESET_KEY_STATUS_MASK;
	key_auto_status = 1;
}
