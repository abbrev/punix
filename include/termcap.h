#ifndef _TERMCAP_H_
#define _TERMCAP_H_

#include <types.h>

extern char PC;
extern char *UP;
extern char *BC;
extern int ospeed;

extern int tgetent(char *__bp, char *__name);
extern int tgetflag(char *__id);
extern int tgetnum(char *__id);
extern char *tgetstr(char *__id, char **__area);

extern void tputs(char *__str, int __affcnt, int (*__putc)(int));
extern char *tgoto(char *__cap, int __col, int __row);
extern char *tparam();	/* VARARGS */

#endif /* _TERMCAP_H_ */
