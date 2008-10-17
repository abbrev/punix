/*
 * dev_vt.c, virtual terminal for Punix
 * Copyright 2007, 2008 Christopher Williams.
 * 
 * This is my first attempt at a VT100 emulator based on the parser state
 * machine at http://vt100.net/emu/dec_ansi_parser
 */

/* FIXME: move global variables to sys/globals.h */

/* FIXME: use the tty structure in each routine to support multiple vt's */

#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "punix.h"
#include "cell.h"
#include "queue.h"
#include "tty.h"
#include "proc.h"
#include "glyph.h"
#include "globals.h"

#define NVT 1

#define WINWIDTH  NUMCELLCOLS
#define WINHEIGHT NUMCELLROWS

#define MARGINTOP 0
#define MARGINBOTTOM (WINHEIGHT-1)
#define MARGINLEFT 0
#define MARGINRIGHT (WINWIDTH-1)

#define MAXPARAMVAL 16383

#define NUMPARAMS 16
#define NUMINTCHARS 2

#if 0
static const struct tty vt[NVT];

static int xon;

static int privflag;
static int nullop;

static char intchars[NUMINTCHARS+1];
static char *intcharp;

static unsigned params[NUMPARAMS];
static int numparams;

static int cursorvisible;

static int tabstops[WINWIDTH];
static struct pos {
	int row, column;
} pos;

struct attrib {
	int bold       : 1;
	int underscore : 1;
	int blink      : 1;
	int reverse    : 1;
};

static struct attrib currattrib;

struct cell {
	struct attrib attrib;
	int c;
};

struct row {
	/*int width;*/
	struct cell cells[WINWIDTH];
};

/* FIXME: make use of the screen array */
static struct row screen[WINHEIGHT];

#endif

#define STGROUND   0 /* ground */
#define STESCAPE   1 /* escape */
#define STESCINT   2 /* escape intermediate */
#define STCSIENT   3 /* csi entry */
#define STCSIPRM   4 /* csi parameter */
#define STCSIINT   5 /* csi intermediate */
#define STCSIIGN   6 /* csi ignore */
#define STDCSENT   7 /* dcs entry */
#define STDCSPRM   8 /* dcs parameter */
#define STDCSINT   9 /* dcs intermediate */
#define STDCSPAS  10 /* dcs passthrough */
#define STDCSIGN  11 /* dcs ignore */
#define STOSCSTR  12 /* osc string */
#define STSOSSTR  13 /* sos/pm/apc string */
#define STLAST    13

struct state {
	void (*entry)(struct tty *);
	void (*event)(int ch, struct tty *);
	void (*exit)(struct tty *);
};

#if 0
static struct state *vtstate;
#endif
static const struct state states[];

/* Transition to a new state */
static void transition(int ch, int newstate,
 void (*trans)(int ch, struct tty *), struct tty *tp)
{
	if (G.vt.vtstate && G.vt.vtstate->exit)
		G.vt.vtstate->exit(tp);
	if (trans)
		trans(ch, tp);
	if (STLAST < (unsigned)newstate)
		return;
	
	G.vt.vtstate = &states[newstate];
	if (G.vt.vtstate->entry)
		G.vt.vtstate->entry(tp);
}

/* XXX: re-write these for Punix */

#if 0
struct glyphset *glyphset;
struct glyphset *charsets[2];
int charset;
#endif

#define SPACEGLYPH (glyphsets[0].lower[' ' - 0x20])

static struct glyphset glyphsets[] = {
#include "glyphsets/uk.inc"
#include "glyphsets/us.inc"
#include "glyphsets/sg.inc"
/* alt char ROM standard chars here */
/* alt char ROM special graphics here */
};

static void invertcursor(struct tty *tp)
{
	(void)tp;
	xorcursor(G.vt.pos.row,
	          G.vt.pos.column > MARGINRIGHT ? MARGINRIGHT : G.vt.pos.column);
}

#if 0
/* move screen contents up */
static void scrolldown(int n)
{
}

/* move screen contents down */
static void scrollup(int n)
{
}
#endif
/* end XXX */

#if 1
static void cursor(struct tty *tp)
{
	if (G.vt.cursorvisible)
		invertcursor(tp);
}
#else
#define cursor(tp) do { if (cursorvisible) invertcursor(); } while (0)
#endif

/* possibly scroll up or down so the cursor is visible again */
static void scroll(struct tty *tp)
{
	if (G.vt.pos.row < 0) {
		scrollup(-G.vt.pos.row);
		G.vt.pos.row = 0;
	} else if (G.vt.pos.row > MARGINBOTTOM) {
		scrolldown(G.vt.pos.row - MARGINBOTTOM);
		G.vt.pos.row = MARGINBOTTOM;
	}
}

static void cmd_ind(struct tty *tp)
{
	++G.vt.pos.row;
	scroll(tp);
}

static void cmd_nel(struct tty *tp)
{
	G.vt.pos.column = 0;
	++G.vt.pos.row;
	scroll(tp);
}

static void cmd_hts(struct tty *tp)
{
	G.vt.tabstops[G.vt.pos.column] = 1;
}

static void cmd_ri(struct tty *tp)
{
	--G.vt.pos.row;
	scroll(tp);
}

static void reset(struct tty *tp)
{
	int i;
	
	G.vt.xon = 1;
	memset((void *)LCD_MEM, 0, 3600); /* XXX constant */
	memset(G.vt.screen, 0, sizeof(G.vt.screen));
	memset(G.vt.tabstops, 0, sizeof(G.vt.tabstops));
	for (i = 0; i < WINWIDTH; i += 8)
		G.vt.tabstops[i] = 1;
	
	G.vt.pos.row = 0;
	G.vt.pos.column = 0;
	G.vt.charset = 0;
	G.vt.charsets[0] = &glyphsets[1];
	G.vt.charsets[1] = &glyphsets[2];
	G.vt.glyphset = G.vt.charsets[G.vt.charset];
	G.vt.cursorvisible = 1;
	
	G.vt.vtstate = NULL;
	transition(0, STGROUND, NULL, tp);
}

/* clear unset parameters up to parameter 'n' */
static void defaultparams(int n, struct tty *tp)
{
	int i;
	
	if (n > NUMPARAMS)
		n = NUMPARAMS;
	
	for (i = G.vt.numparams; i < n; ++i)
		G.vt.params[i] = 0;
	
	G.vt.numparams = n;
}

/******************************************************************************
 * Actions
 * 
 * These are implemented according to the descriptions given in the web page
 * referenced at the top of this file and to observations of behaviour of "real"
 * terminal emulators like xterm and rxvt.
 ******************************************************************************/

static void print(int ch, struct tty *tp)
{
	/* Display a glyph according to the character set mappings and shift
	 * states in effect. */
	
	/* NB: This defers wrapping to the next line until the cursor is past
	 * the right margin. This means that a newline character will wrap
	 * correctly and will not create a blank line if it arrives after the
	 * character in the right column is printed.
	 * 
	 * Useful information: xterm and rxvt keep the cursor in the right
	 * margin and set a flag when the end of line has been reached. This
	 * means that cursor movement escape codes reset the "end of line" flag,
	 * and movement is relative to the right margin. On the other hand,
	 * gnome-terminal and konsole move the cursor to one column beyond the
	 * right margin. This means that only the "print" routine (such as the
	 * one we're in now) needs to deal with this wrap-around case, and also
	 * movement is relative to the column beyond the right margin.
	 * 
	 * I chose the latter behaviour. However, the cursor is still drawn in
	 * the right margin, even if it's outside the margin (see the
	 * "invertcursor" routine).
	 */
	if (G.vt.pos.column > MARGINRIGHT) {
		++G.vt.pos.row;
		G.vt.pos.column = 0;
		scroll(tp);
	}
	
	/* draw the glyph */
	if (ch < 0x80)
		drawglyph(&G.vt.glyphset->lower[ch - 0x20], G.vt.pos.row, G.vt.pos.column);
	else
		drawglyph(&G.vt.glyphset->upper[ch - 0x20 - 0x80], G.vt.pos.row, G.vt.pos.column);
	
	++G.vt.pos.column;
}

static void execute(int ch, struct tty *tp)
{
	/* Execute the C0 or C1 control function. */
	switch (ch) {
	case 0x05: /* ENQ */
		/* transmit answerback message (?) */
		break;
	case 0x08: /* BS */
		if (G.vt.pos.column > 0)
			--G.vt.pos.column;
		break;
	case 0x09: /* HT */
		/* move the cursor to the next tab stop or the right margin */
		while (G.vt.pos.column < MARGINRIGHT && !G.vt.tabstops[++G.vt.pos.column])
			;
		break;
	case 0x0a: /* LF */
	case 0x0b: /* VT */
	case 0x0c: /* FF */
		/* new line operation */
		++G.vt.pos.row;
		scroll(tp);
		/* fall through */
	case 0x0d: /* CR */
		/* move cursor to the left margin on the current line */
		G.vt.pos.column = 0;
		break;
	case 0x0e: /* SO */
		/* invoke G1 character set, as designated by SCS control
		 * sequence */
		G.vt.charset = 1;
		G.vt.glyphset = G.vt.charsets[G.vt.charset];
		break;
	case 0x0f: /* SI */
		/* select G0 character set, as selected by ESC ( sequence */
		G.vt.charset = 0;
		G.vt.glyphset = G.vt.charsets[G.vt.charset];
		break;
	case 0x11: /* XON */
		/* resume transmission */
		G.vt.xon = 1;
		break;
	case 0x13: /* XOFF */
		/* stop transmitting all codes except XOFF and XON */
		G.vt.xon = 0;
		break;
	/* C1 control codes */
	case 'H'+0x40:
		/* HTS - Horizontal Tabulation Set */
		cmd_hts(tp);
		break;
	case 'D'+0x40:
		/* IND - Index */
		cmd_ind(tp);
		break;
	case 'M'+0x40:
		/* RI - Reverse Index */
		cmd_ri(tp);
		break;
	case 'E'+0x40:
		/* NEL - Next Line */
		cmd_nel(tp);
		break;
	}
}

static void clear(struct tty *tp)
{
	/* Clear the current private flag, intermediate characters, final
	 * character, and parameters. */
	
	G.vt.privflag = 0;
	G.vt.nullop = 0;
	
	G.vt.intcharp = &G.vt.intchars[0];
	G.vt.intchars[0] = '\0';
	
	G.vt.numparams = 1;
	G.vt.params[0] = 0;
}

static void collect(int ch, struct tty *tp)
{
	/* Store the private marker or intermediate character for later use in
	 * selecting a control function to be executed when a final character
	 * arrives. */
	
	if (G.vt.intcharp >= &G.vt.intchars[NUMINTCHARS]) {
		G.vt.nullop = 1; /* flag this so the dispatch turns into a no-op */
		return;
	}
	
	*G.vt.intcharp++ = ch;
	*G.vt.intcharp = '\0';
}

static void param(int ch, struct tty *tp)
{
	/* Collect the characters of a parameter string for a control sequence
	 * or device control sequence and build a list of parameters. The
	 * characters processed by this action are the digits 0-9 and the
	 * semicolon. The semicolon separates parameters. A maximum of 16
	 * parameters need be stored. If more than 16 parameters arrive, all the
	 * extra parameters are silently ignored. */
	
	if (G.vt.numparams > NUMPARAMS)
		return;
	
	if (ch == ';') {
		if (G.vt.numparams <= NUMPARAMS)
			G.vt.params[G.vt.numparams++] = 0;
	} else {
		G.vt.params[G.vt.numparams-1] = G.vt.params[G.vt.numparams-1] * 10 + (ch - '0');
		if (G.vt.params[G.vt.numparams-1] > MAXPARAMVAL)
			G.vt.params[G.vt.numparams-1] = MAXPARAMVAL;
	}
}

static void esc_dispatch(int ch, struct tty *tp)
{
	/* Determine the control function to be executed from the intermediate
	 * character(s) and final character, and execute it. The intermediate
	 * characters are available because collect() stored them as they
	 * arrived. */
	
	if (G.vt.nullop)
		return;
	
	switch (ch) {
#if 0
	case '8':
		/* DECALN # (DEC Private) */
		/* DECRC (DEC Private) */
		break;
	case '3':
		/* DECDHL # (DEC Private) */
		/* top half */
		break;
	case '4':
		/* DECDHL # (DEC Private) */
		/* bottom half */
		break;
	case '6':
		/* DECDWL # (DEC Private) */
		break;
	case 'Z':
		/* DECID (DEC Private) */
		break;
	case '=':
		/* DECKPAM (DEC Private) */
		break;
	case '>':
		/* DECKPNM (DEC Private) */
		break;
	case '7':
		/* DECSC (DEC Private) */
		break;
	case '5':
		/* DECSWL # (DEC Private) */
		break;
#endif
	case 'H':
		/* HTS - Horizontal Tabulation Set */
		cmd_hts(tp);
		break;
	case 'D':
		/* IND - Index */
		cmd_ind(tp);
		break;
	case 'M':
		/* RI - Reverse Index */
		cmd_ri(tp);
		break;
	case 'E':
		/* NEL - Next Line */
		cmd_nel(tp);
		break;
	case 'c':
		/* RIS - Reset to Initial State */
		reset(tp);
		break;
	case 'A':
		/* SCS - Select Character Set (UK) */
		if (G.vt.intchars[0] == '(')
			G.vt.charsets[0] = &glyphsets[0];
		else if (G.vt.intchars[0] == ')')
			G.vt.charsets[1] = &glyphsets[0];
		
		G.vt.glyphset = G.vt.charsets[G.vt.charset];
		break;
	case 'B':
		/* SCS - Select Character Set (ASCII) */
		if (G.vt.intchars[0] == '(')
			G.vt.charsets[0] = &glyphsets[1];
		else if (G.vt.intchars[0] == ')')
			G.vt.charsets[1] = &glyphsets[1];
		
		G.vt.glyphset = G.vt.charsets[G.vt.charset];
		break;
	case '0':
		/* SCS (Special Graphics) */
		if (G.vt.intchars[0] == '(')
			G.vt.charsets[0] = &glyphsets[2];
		else if (G.vt.intchars[0] == ')')
			G.vt.charsets[1] = &glyphsets[2];
		
		G.vt.glyphset = G.vt.charsets[G.vt.charset];
		break;
	case '1':
		/* SCS (Alternate Character ROM Standard Character Set) */
		break;
	case '2':
		/* SCS (Alternate Character ROM Special Graphics) */
		break;
	}
}

static void csi_dispatch(int ch, struct tty *tp)
{
	/* Determine the control function to be executed from private marker,
	 * intermediate character(s) and final character, and execute it,
	 * passing in the parameter list. The private marker and intermediate
	 * characters are available because collect() stored them as they
	 * arrived. */
	
	int n, r, c;
	int i;
	
	if (G.vt.nullop)
		return;
	
	switch (ch) {
	case 'A':
		/* CUU Pn - Cursor Up */
		n = G.vt.params[0];
		if (n == 0)
			n = 1;
		
		G.vt.pos.row -= n;
		if (G.vt.pos.row < 0)
			G.vt.pos.row = 0;
		break;
	case 'B':
		/* CUD Pn - Cursor Down */
		n = G.vt.params[0];
		if (n == 0)
			n = 1;
		
		G.vt.pos.row += n;
		if (G.vt.pos.row > MARGINBOTTOM)
			G.vt.pos.row = MARGINBOTTOM;
		break;
	case 'C':
		/* CUF Pn - Cursor Forward */
		n = G.vt.params[0];
		if (n == 0)
			n = 1;
		
		G.vt.pos.column += n;
		if (G.vt.pos.column > MARGINRIGHT)
			G.vt.pos.column = MARGINRIGHT;
		break;
	case 'D':
		/* CUB Pn - Cursor Backward */
		n = G.vt.params[0];
		if (n == 0)
			n = 1;
		
		G.vt.pos.column -= n;
		if (G.vt.pos.column < 0)
			G.vt.pos.column = 0;
		break;
	case 'H':
		/* CUP Pr Pc - Cursor Position */
	case 'f':
		/* HVP Pn Pn - Horizontal and Vertical Position */
		defaultparams(2, tp);
		r = G.vt.params[0];
		c = G.vt.params[1];
		
		G.vt.pos.row = r - 1;
		if (G.vt.pos.row < 0)
			G.vt.pos.row = 0;
		else if (G.vt.pos.row > MARGINBOTTOM)
			G.vt.pos.row = MARGINBOTTOM;
		
		G.vt.pos.column = c - 1;
		if (G.vt.pos.column < 0)
			G.vt.pos.column = 0;
		else if (G.vt.pos.column > MARGINRIGHT)
			G.vt.pos.column = MARGINRIGHT;
		break;
	case 'c':
		/* DA Pn - Device Attributes */
		/* FIXME: send a response to the host */
		break;
	case 'q':
		/* DECLL Ps (DEC Private) */
		break;
	case 'x':
		/* DECREQTPARM sol */
		/* DECREPTPARM sol par nbits xspeed rspeed clkmul flags */
		/* FIXME */
		break;
	case 'r':
		/* DECSTBM Pn Pn (DEC Private) */
		break;
	case 'y':
		/* DECTST 2 Ps */
#if 0
		defaultparams(2, tp);
		if (G.vt.params[0] != 2)
			break;
		
		n = G.vt.params[1];
		
		while (n & 8) /* Repeat Selected Test(s) indefinitely */
			if (n == 0)
				; /* reset the VT100 */
			else if (n & 1)
				; /* Power up self-test */
			else if (n & 2)
				; /* Data Loop Back */
			else if (n & 4)
				; /* EIA modem control test */
#endif
		break;
	case 'n':
		/* DSR Ps */
		/* Device Status Report */
		break;
	case 'J':
		/* ED Ps */
		/* Erase in Display */
		n = G.vt.params[0];
		
		if (n == 0) {
			/* Erase from the active position to the end of the
			 * screen, inclusive (default) */
			for (c = G.vt.pos.column; c < WINWIDTH; ++c)
				drawglyph(&SPACEGLYPH, G.vt.pos.row, c);
			for (r = G.vt.pos.row + 1; r < WINHEIGHT; ++r)
				for (c = 0; c < WINWIDTH; ++c)
					drawglyph(&SPACEGLYPH, r, c);
		} else if (n == 1) {
			/* Erase from start of the screen to the active
			 * position, inclusive */
			for (r = 0; r < G.vt.pos.row; ++r)
				for (c = 0; c < WINWIDTH; ++c)
					drawglyph(&SPACEGLYPH, r, c);
			for (c = 0; c <= G.vt.pos.column; ++c)
				drawglyph(&SPACEGLYPH, G.vt.pos.row, c);
		} else if (n == 2) {
			/* XXX: we could just erase the entire display,
			 * excluding the status line at the bottom */
			/* Erase all of the display -- all lines are erased,
			 * changed to single-width, and the cursor does not
			 * move. */
			for (r = 0; r < WINHEIGHT; ++r)
				for (c = 0; c < WINWIDTH; ++c)
					drawglyph(&SPACEGLYPH, r, c);
		}
		break;
	case 'K':
		/* EL Ps */
		/* Erase in Line */
		n = G.vt.params[0];
		
		if (n == 0) {
			/* Erase from the active position to the end of the
			 * line, inclusive (default) */
			for (c = G.vt.pos.column; c < WINWIDTH; ++c)
				drawglyph(&SPACEGLYPH, G.vt.pos.row, c);
		} else if (n == 1) {
			/* Erase from the start of the line to the active
			 * position, inclusive */
			for (c = 0; c <= G.vt.pos.column; ++c)
				drawglyph(&SPACEGLYPH, G.vt.pos.row, c);
		} else if (n == 2) {
			/* Erase all of the line, inclusive */
			for (c = 0; c < WINWIDTH; ++c)
				drawglyph(&SPACEGLYPH, G.vt.pos.row, c);
		}
		break;
	case 'h':
		/* SM Ps ... */
		/* Set Mode */
		
		for (i = 0; i < G.vt.numparams; ++i)
			switch (G.vt.params[i]) {
			case 1:  /* DECCKM */
				break;
			case 2:  /* DECCANM */
				break;
			case 3:  /* DECCOLM */
				break;
			case 4:  /* DECSCLM */
				break;
			case 5:  /* DECSCNM */
				break;
			case 6:  /* DECOM */
				break;
			case 7:  /* DECAWM */
				break;
			case 8:  /* DECARM */
				break;
			case 9:  /* DECINLM */
				break;
			}
		
		break;
	case 'l':
		/* RM Ps ... */
		/* Reset Mode */
		
		for (i = 0; i < G.vt.numparams; ++i)
			switch (G.vt.params[i]) {
			case 1:  /* DECCKM */
				break;
			case 2:  /* DECCANM */
				break;
			case 3:  /* DECCOLM */
				break;
			case 4:  /* DECSCLM */
				break;
			case 5:  /* DECSCNM */
				break;
			case 6:  /* DECOM */
				break;
			case 7:  /* DECAWM */
				break;
			case 8:  /* DECARM */
				break;
			case 9:  /* DECINLM */
				break;
			case 20: /* LNM */
				break;
			}
		
		break;
	case 'm':
		/* SGR Ps ... */
		/* FIXME: Select Graphic Rendition */
		
		for (i = 0; i < G.vt.numparams; ++i) {
			switch (G.vt.params[i]) {
			case 0:
				/* attributes off */
				break;
			case 1:
				/* bold or increased intensity */
				break;
			case 4:
				/* underscore */
				break;
			case 5:
				/* blink */
				break;
			case 7:
				/* negative (reverse image) */
				;
			}
		}
		
		break;
	case 'g':
		/* TBC Ps - Tabulation Clear */
		/* FIXME */
		switch (G.vt.params[0]) {
		case 0:
			/* Clear the horizontal tab stop at the active
			 * position (default) */
			G.vt.tabstops[G.vt.pos.column] = 0;
			break;
#if 0
		case 1:
			/* Clear vertical tab stop at active line */
			break;
		case 2:
			/* Clear all horizontal tab stops in active line */
			break;
#endif
		case 3:
			/* Clear all horizontal tab stops */
			for (i = 0; i < 60; ++i) G.vt.tabstops[i] = 0;
			break;
#if 0
		case 4:
			/* Clear all vertical tab stops */
			break;
#endif
		}
		
		break;
	default:
		/* error ? */
		;
	}
}

/* NOTE: The following actions will probably be a no-op in this vt. */

static void hook(struct tty *tp)
{
	/* FIXME: Determine the control function from the private marker,
	 * intermediate character(s) and final character, and execute it,
	 * passing in the parameter list. Also select a handler function for the
	 * rest of the characters in the control string. This handler function
	 * will be called by the put() action for every character in the control
	 * string as it arrives.
	 * 
	 * This way of handling device control strings has been selected because
	 * it allows the simple plugging-in of extra parsers as functionality is
	 * added. Support for a fairly simple control string like DECDLD
	 * (Downline Load) could be added into the main parser if soft
	 * characters were required, but the main parser is no place for
	 * complicated protocols like ReGIS. */
}

static void put(int ch, struct tty *tp)
{
	/* FIXME: Pass characters from the data string part of a device control
	 * string to a handler that has previously been selected by the hook()
	 * action. C0 controls are also passed to the handler. */
}

static void unhook(struct tty *tp)
{
	/* FIXME: Call the previously selected handler function with an "end of
	 * data" parameter. This allows the handler to finish neatly. */
}

static void osc_start(struct tty *tp)
{
	/* FIXME: Initialize an external parser (the "OSC Handler") to handle
	 * the characters from the control string. OSC control strings are not
	 * structured in the same way as device control strings, so there is no
	 * choice of parsers. */
}

static void osc_put(int ch, struct tty *tp)
{
	/* FIXME: Pass characters from the control string to the OSC Handler as
	 * they arrive. There is therefore no need to buffer characters until
	 * the end of the control string is recognised. */
}

static void osc_end(struct tty *tp)
{
	/* FIXME: Allow the OSC Handler to finish neatly. */
}

/***************************************
 * State events
 ***************************************/

static void ground_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (c <= 0x7f)
		print(ch, tp);
}

static void escape_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (c <= 0x2f)
		transition(c, STESCINT, collect, tp);
	else if (c == 0x50)
		transition(c, STDCSENT, NULL, tp);
	else if (c == 0x5b)
		transition(c, STCSIENT, NULL, tp);
	else if (c == 0x5d)
		transition(c, STOSCSTR, NULL, tp);
	else if (c == 0x58 || c == 0x5e || c == 0x5f)
		transition(c, STSOSSTR, NULL, tp);
	else if (c <= 0x7e)
		transition(c, STGROUND, esc_dispatch, tp);
}

static void escint_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (c <= 0x2f)
		collect(c, tp);
	else if (c <= 0x7e)
		transition(c, STGROUND, esc_dispatch, tp);
}

static void csient_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (c <= 0x2f)
		transition(c, STCSIINT, collect, tp);
	else if (c == 0x3a)
		transition(c, STCSIIGN, NULL, tp);
	else if (c <= 0x3b)
		transition(c, STCSIPRM, param, tp);
	else if (c <= 0x3f)
		transition(c, STCSIPRM, collect, tp);
	else if (c <= 0x7e)
		transition(c, STGROUND, csi_dispatch, tp);
}

static void csiprm_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (c <= 0x2f)
		transition(c, STCSIINT, collect, tp);
	else if (c <= 0x39 || c == 0x3b)
		param(c, tp);
	else if (c <= 0x3f)
		transition(c, STCSIIGN, NULL, tp);
	else if (c <= 0x7e)
		transition(c, STGROUND, csi_dispatch, tp);
}

static void csiint_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (c <= 0x2f)
		collect(c, tp);
	else if (c <= 0x7e)
		transition(c, STGROUND, csi_dispatch, tp);
}

static void csiign_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (ch <= 0x1f)
		execute(ch, tp);
	else if (0x40 <= c && c <= 0x7e)
		transition(c, STGROUND, NULL, tp);
}

static void dcsent_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (0x20 <= c && c <= 0x2f)
		transition(c, STDCSINT, collect, tp);
	else if (c == 0x3a)
		transition(c, STDCSIGN, NULL, tp);
	else if (c <= 0x3b)
		param(c, tp);
	else if (c <= 0x3f)
		transition(c, STDCSPRM, collect, tp);
	else if (c <= 0x7e)
		transition(c, STDCSPAS, NULL, tp);
}

static void dcsprm_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (0x20 <= c && c <= 0x2f)
		transition(c, STDCSINT, collect, tp);
	else if (c <= 0x39 || c == 0x3b)
		param(c, tp);
	else if (c <= 0x3f)
		transition(c, STDCSIGN, NULL, tp);
	else if (c <= 0x7e)
		transition(c, STDCSPAS, NULL, tp);
}

static void dcsint_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (0x20 <= c && c <= 0x2f)
		collect(c, tp);
	else if (c <= 0x3f)
		transition(c, STDCSIGN, NULL, tp);
	else if (c <= 0x7e)
		transition(c, STDCSPAS, NULL, tp);
}

static void dcspas_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (c <= 0x7e)
		put(ch, tp);
}

static void dcsign_event(int ch, struct tty *tp)
{
}

static void oscstr_event(int ch, struct tty *tp)
{
	int c = ch & 0x7f;
	
	if (0x20 <= c && c <= 0x7f)
		osc_put(ch, tp);
}

static void sosstr_event(int ch, struct tty *tp)
{
	if (ch == 0x07)
		transition(ch, STGROUND, NULL, tp);
}

static const struct state states[] = {
	{ NULL, ground_event, NULL }, /* ground */
	{ clear, escape_event, NULL }, /* escape */
	{ NULL, escint_event, NULL }, /* escape intermediate */
	{ NULL, csient_event, NULL }, /* csi entry */
	{ NULL, csiprm_event, NULL }, /* csi parameter */
	{ NULL, csiint_event, NULL }, /* csi intermediate */
	{ NULL, csiign_event, NULL }, /* csi ignore */
	{ clear, dcsent_event, NULL }, /* dcs entry */
	{ NULL, dcsprm_event, NULL }, /* dcs parameter */
	{ NULL, dcsint_event, NULL }, /* dcs intermediate */
	{ hook, dcspas_event, unhook }, /* dcs passthrough */
	{ NULL, dcsign_event, NULL }, /* dcs ignore */
	{ osc_start, oscstr_event, osc_end }, /* osc string */
	{ NULL, sosstr_event, NULL }, /* sos/pm/apc string */
};

/***************************************
 * vt device interface
 ***************************************/

/* one-shot routine on system startup */
void vtinit()
{
	G.vt.vtstate = &states[STGROUND];
	reset(&G.vt.vt[0]); /* XXX dev */
	cursor(&G.vt.vt[0]);
	G.vt.key_repeat_start_delay = 256 / 4;
	G.vt.key_repeat_delay = 256 / 20;
	G.vt.key_compose = 0;
	G.vt.key_mod_sticky = 0;
}

/*
 * vtoutput is similar to ttyoutput, but it bypasses the t_outq FIFO, handling
 * all terminal activity in the same context as the current process.
 */
/* FIXME: use the tty structure */
void vtoutput(int ch, struct tty *tp)
{
	int x = spl5();
	cursor(tp);
	
	/* process the "anywhere" pseudo-state */
	if (ch == 0x1b)
		transition(ch, STESCAPE, NULL, tp);
	else if (ch == 0x90)
		transition(ch, STDCSENT, NULL, tp);
	else if (ch == 0x98 || ch == 0x9e || ch == 0x9f)
		transition(ch, STSOSSTR, NULL, tp);
	else if (ch == 0x9b)
		transition(ch, STCSIENT, NULL, tp);
	else if (ch == 0x9c)
		transition(ch, STGROUND, NULL, tp);
	else if (ch == 0x9d)
		transition(ch, STOSCSTR, NULL, tp);
	else if (ch == 0x18 || ch == 0x1a ||
	         (0x80 <= ch && ch <= 0x9a))
		transition(ch, STGROUND, execute, tp);
	else
		G.vt.vtstate->event(ch, tp);
	
	cursor(tp);
	splx(x);
}

/*
 * vtinput is similar to ttyinput, but it is
 * designed for the needs of the vt device.
 */
void vtinput(int ch, struct tty *tp)
{
	int iflag = tp->t_termios.c_iflag;
	int lflag = tp->t_termios.c_lflag;
	
	if ((ch &= 0x7f) == '\r' && iflag & ICRNL)
		ch = '\n';
	if ((lflag & ICANON) && (ch == tp->t_termios.c_cc[VQUIT] ||
	  ch == tp->t_termios.c_cc[VINTR])) {
		gsignal(tp->t_pgrp, ch == tp->t_termios.c_cc[VQUIT] ? SIGINT : SIGQUIT);
		flushtty(tp);
		return;
	}
	if (qisfull(&tp->t_rawq)) {
		flushtty(tp);
		return;
	}
	putc(ch, &tp->t_rawq);
	if ((lflag & ICANON) == 0 || ch == '\n' || ch == 004) {
		wakeup(&tp->t_rawq);
		if (putc(0xff, &tp->t_rawq) >= 0)
			++tp->t_delct;
	}
	if (lflag & ECHO)
		vtoutput(ch, tp);
}

/* NB: there is no vtxint routine */

void vtrint(dev_t dev)
{
	int ch = G.vt.key;
	
#if 0
	vtinput(ch, &G.vt.vt[MINOR(dev)]);
#else
	vtoutput(ch, &G.vt.vt[MINOR(dev)]); /* XXX for debugging/testing */
#endif
}

int kputchar(int ch)
{
	struct tty *tp = &G.vt.vt[0]; /* XXX */
	ch &= 0xff;
	vtoutput(ch, tp);
	return ch;
}

void vtopen(dev_t dev, int rw)
{
	int minor = MINOR(dev);
	struct tty *tp;
	
	if (minor >= NVT) {
		P.p_error = ENXIO;
		return;
	}
	tp = &G.vt.vt[minor];
	if (!(tp->t_state & ISOPEN)) {
		tp->t_state = ISOPEN;
		tp->t_termios.c_iflag = ICRNL;
		tp->t_termios.c_oflag = 0;
		tp->t_termios.c_cflag = CS8;
		tp->t_termios.c_lflag = ISIG|ICANON|ECHO;
		ttychars(tp);
	}
	ttyopen(dev, tp);
}

void vtclose(dev_t dev, int rw)
{
}

void vtread(dev_t dev)
{
	ttyread(&G.vt.vt[MINOR(dev)]);
}

void vtwrite(dev_t dev)
{
	struct tty *tp = &G.vt.vt[MINOR(dev)];
	int ch;
	
	if (!(tp->t_state & ISOPEN))
		return;
	
	while ((ch = cpass()) >= 0)
		vtoutput(ch, tp);
}

void vtioctl(dev_t dev, int cmd, void *cmarg, int rw)
{
}
