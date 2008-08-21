#ifndef _STDDEF_H_
#define _STDDEF_H_

/* $Id: stddef.h,v 1.1 2007/05/03 08:59:58 fredfoobar Exp $ */

/* POSIX compliant? */

/* sys/types.h defines size_t */
#include <sys/types.h>

#define NULL	((void *)0)
#define offsetof(type,member)	((size_t) &((type *)0)->member - (type *)0)

typedef long	ptrdiff_t;
typedef char	wchar_t;	/* XXX */

#endif /* _STDDEF_H_ */
