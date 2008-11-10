#ifndef _STDARG_H_
#define _STDARG_H_

/* $Id$ */

/*
 * POSIX compliant, except for __FAST_VA_ARG__.
 * 
 * If __FAST_VA_ARG__ is defined, the faster and smaller macros are used.
 * They ignore alignment issues altogether. The standard specifies that the type
 * given to va_arg must be the promoted type anyway, not the actual type of the
 * argument.
 */

typedef void *va_list;

#define va_start(ap, argN)	\
 ((void)((ap)=(va_list)((char*)(&argN)+((sizeof(argN)+1)&~1))))

#define va_copy(dest, src)	((void)((dest)=(src)))

#define va_end(ap)		((void)0)

#ifdef __FAST_VA_ARG__
# define va_arg(ap, type)	(*(((type*)(ap))++))
#else /* __FAST_VA_ARG__ */
# define va_arg(ap, type)	\
 (*(type*)((((char*)(ap))+=((sizeof(type)+1)&~1))-((sizeof(type)+1)&~1)))
#endif

#endif /* _STDARG_H_ */
