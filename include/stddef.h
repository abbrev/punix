#ifndef _STDDEF_H_
#define _STDDEF_H_

/* $Id$ */

/* POSIX compliant? */

/* sys/types.h defines size_t */
#include <sys/types.h>

#define NULL	((void *)0)
#define offsetof(type,member)	((size_t) &((type *)0)->member - (type *)0)

typedef long	ptrdiff_t;
typedef char	wchar_t;	/* XXX */

#endif /* _STDDEF_H_ */
