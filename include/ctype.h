#ifndef _CTYPE_H_
#define _CTYPE_H_

/* $Id$ */

/* this should be POSIX compliant */

int	isalnum(int __c);
int	isalpha(int __c);
int	isascii(int __c);
int	isblank(int __c);
int	iscntrl(int __c);
int	isdigit(int __c);
int	isgraph(int __c);
int	islower(int __c);
int	isprint(int __c);
int	ispunct(int __c);
int	isspace(int __c);
int	isupper(int __c);
int	isxdigit(int __c);

int	toascii(int __c);

int	tolower(int __c);
int	toupper(int __c);

int	_tolower(int __c);
int	_toupper(int __c);

extern const unsigned char _ctype[];

#define _U	0x01
#define _L	0x02
#define _N	0x04
#define _S	0x08
#define _P	0x10
#define _C	0x20
#define _X	0x40

#define isalpha(c)	(_ctype[(unsigned char)(c)]&(_U|_L))
#define isupper(c)	(_ctype[(unsigned char)(c)]&_U)
#define islower(c)	(_ctype[(unsigned char)(c)]&_L)
#define isdigit(c)	(_ctype[(unsigned char)(c)]&_N)
#define isxdigit(c)	(_ctype[(unsigned char)(c)]&(_N|_X))
#define isspace(c)	(_ctype[(unsigned char)(c)]&_S)
#define ispunct(c)	(_ctype[(unsigned char)(c)]&_P)
#define isalnum(c)	(_ctype[(unsigned char)(c)]&(_U|_L|_N))
#define isprint(c)	(_ctype[(unsigned char)(c)]&(_P|_U|_L|_N|_S))
#define isgraph(c)	(_ctype[(unsigned char)(c)]&(_P|_U|_L|_N))
#define iscntrl(c)	(_ctype[(unsigned char)(c)]&_C)
#define isascii(c)	(!((c)&~0x7f))
#define toascii(c)	((c)&0x7f)

#define _toupper(c)	((c)^0x20)
#define _tolower(c)	((c)^0x20)

inline int tolower(int c) { if (isupper(c)) return tolower(c); return c; }
inline int toupper(int c) { if (islower(c)) return toupper(c); return c; }

#if 0
/* another definition of _toupper/_tolower with different undefined behaviour */
#define _toupper(c)	((c)+'A'-'a')
#define _tolower(c)	((c)+'a'-'A')

/* yet another definition of _toupper/_tolower */
#define _toupper(c)	((c)&~0x20)
#define _tolower(c)	((c)|0x20)
#endif

#endif /* _CTYPE_H_ */
