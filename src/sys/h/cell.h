#ifndef _CELL_H_
#define _CELL_H_

#include "calc.h"
#include "lcd.h"
/* #include "cellglo.h" */

#define TABSTOP 8

#define textcell(r,c) (G_cells[60*(r) + (c)])

#if CALC_HAS_LARGE_SCREEN
	#define	NUMCELLROWS	20
	#define	NUMCELLCOLS	60
#else
	#define	NUMCELLROWS	16
	#define	NUMCELLCOLS	40
#endif

#endif
