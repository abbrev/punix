#ifndef _CELL_H_
#define _CELL_H_

#include "lcd.h"
/* #include "cellglo.h" */

#define TABSTOP 8

#define textcell(r,c) (G_cells[60*(r) + (c)])

#ifdef TI89
	#define	NUMCELLROWS	16
	#define	NUMCELLCOLS	40
#else
	#define	NUMCELLROWS	20
	#define	NUMCELLCOLS	60
#endif

#endif
