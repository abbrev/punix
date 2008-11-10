#ifndef _STDLIB_H_
#define _STDLIB_H_

/* $Id$ */

/* kind of POSIX compliant */

#include <stddef.h>
#include <sys/wait.h>

#define EXIT_FAILURE	(-1)
#define EXIT_SUCCESS	0

#define RAND_MAX	32767	/* should this be 65535? */
#define MB_CUR_MAX	1

/* Returned by `div' */
typedef struct {
	int	quot;	/* Quotient */
	int	rem;	/* Remainder */
} div_t;

/* Returned by `ldiv' */
typedef struct {
	long	quot;	/* Quotient */
	long	rem;	/* Remainder */
} ldiv_t;

/* Returned by `lldiv' */
typedef struct {
	long long	quot;	/* Quotient */
	long long	rem;	/* Remainder */
} lldiv_t;

void		_Exit(int __status);
void		abort(void);
int		abs(int __j);
int		atexit(void (*__function)(void));
double		atof(const char *__nptr);
int		atoi(const char *__nptr);
long		atol(const char *__nptr);
long long	atoll(const char *__nptr);
void		*bsearch(const void *__key, const void *__base, size_t __nmemb,
		size_t __size, int (*__compar)(const void *, const void *));
void		*calloc(size_t __nmemb, size_t __size);
div_t		div(int __numer, int __denom);
void		exit(int __status);
void		free(void *__ptr);
char		*getenv(const char *);
long		labs(long __j);
ldiv_t		ldiv(long __numer, long __denom);
long long	llabs(long long __j);
lldiv_t		lldiv(long long __numer, long long __denom);
void		*malloc(size_t __size);
int		mblen(const char *__s, size_t __n);
size_t		mbstowcs(wchar_t *__dest, const char *__src, size_t __n);
int		mbtowc(wchar_t *, const char *, size_t);
void		qsort(void *, size_t, size_t,
			int (*)(const void *, const void *));
int		rand(void);
void		*realloc(void *__ptr, size_t __size);
int		setenv(const char *__name, const char *__value,
		int __overwrite);
void		srand(unsigned __seed);
double		strtod(const char *__nptr, char **__endptr);
float		strtof(const char *__nptr, char **__endptr);
long		strtol(const char *__nptr, char **__endptr, int __base);
long double	strtold(const char *__nptr, char **__endptr);
long long	strtoll(const char *__nptr, char **__endptr, int __base);
unsigned long	strtoul(const char *__nptr, char **__endptr, int __base);
unsigned long long	strtoull(const char *__nptr, char **__endptr,
		int __base);
int		system(const char *__string);
int		unsetenv(const char *__name);
size_t		wcstombs(char *__dest, const wchar_t *__src, size_t __n);
int		wctomb(char *__s, wchar_t __wc);

/* [XSI] */
/*
double		drand48(void);
double		erand48(unsigned short[3]);
int		getsubopt(char **, char *const *, char **);
int		grantpt(int);
char		*initstate(unsigned, char *, size_t);
long		jrand48(unsigned short[3]);
char		*l64a(long);
void		lcong48(unsigned short[7]);
long		lrand48(void);
int		mkstemp(char *);
long		mrand48(void);
long		nrand48(unsigned short[3]);
int		posix_openpt(int);
char		*ptsname(int);
int		putenv(char *);
long		random(void);
char		*realpath(const char *, char *);
unsigned short	seed48(unsigned short[3]);
void		setkey(const char *);
char		*setstate(const char *);
void		srand48(long);
void		srandom(unsigned);
int		unlockpt(int);
*/

#endif /* _STDLIB_H_ */
