/*
 * currently just a hack to get characters on the screen.
 * this will be deprecated by dev_vt when it's finished.
 */

#include <sys/types.h>

#include "punix.h"
#include "scrollcell.h"
#include "cell.h"
#include "drawcell.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

STARTUP(static void scroll())
{
	if (G.cellrow >= NUMCELLROWS) {
		scrollcellup(NUMCELLROWS - G.cellrow + 1);
		G.cellrow = NUMCELLROWS - 1;
	} else if (G.cellrow < 0) {
		scrollcelldown(-G.cellrow);
		G.cellrow = 0;
	}
}

STARTUP(static void wrap())
{
	/* wrap around to the next line */
	if (G.cellcol >= NUMCELLCOLS) {
		++G.cellrow;
		G.cellcol = 0;
	}
	
	scroll();
}

STARTUP(int kputchar(int c))
{
	c &= 0xFF;
	
	switch (c) {
	case '\n':
		++G.cellrow;
		scroll();
		/* fall through */
	case '\r':
		G.cellcol = 0;
		break;
	case '\t':
		wrap();
		
		do {
			//textcell(G.cellrow, G.cellcol) = ' ';
			drawcell(G.cellrow, G.cellcol, ' ');
		} while ((++G.cellcol) % TABSTOP);
		break;
	case '\f':
	case '\v':
		++G.cellrow;
		scroll();
		break;
	case '\b':
		if (G.cellcol == 0)
			break;
		
		--G.cellcol;
		
		break;
	default:
		wrap();
		
		//textcell(G.cellrow, G.cellcol) = c;
		drawcell(G.cellrow, G.cellcol, c);
		++G.cellcol;
	}
	
	return c;
}
