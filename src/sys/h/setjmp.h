#ifndef _H_SETJMP_H_
#define _H_SETJMP_H_

/* $Id: setjmp.h,v 1.4 2008/01/15 20:23:56 fredfoobar Exp $ */

typedef struct {
	long	d[5];	/* %d3-%d7 */
	void *	usp;
	long	a[5];	/* %a2-%a6 */
	void *	sp;		/* %a7 */
	void *	pc;		/* return address */
} jmp_buf[1];

int	setjmp(jmp_buf __env);
void	longjmp(jmp_buf __env, int __val);

#endif
