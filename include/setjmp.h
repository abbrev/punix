#ifndef _SETJMP_H_
#define _SETJMP_H_

/* $Id$ */

typedef struct {
	long	reg[10];	/* %d3-%d7/%a2-%a6 */
	long	sp;		/* %a7 */
	long	retaddr;	/* return address */
} jmp_buf[1];

int	setjmp(jmp_buf __env);
void	longjmp(jmp_buf __env, int __val);

/*
typedef struct {
	long	reg[11];
	long	sp;
	long	retaddr;
	sigset_t	sigs;
} sigjmp_buf[1];

int	sigsetjmp(sigjmp_buf __env, int __savesigs);
void	siglongjmp(sigjmp_buf __env, int __val);
*/

#endif
