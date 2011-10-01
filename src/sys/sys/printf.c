/*	printf() - printf() for user program		Author: Kees J. Bot
 *								15 Jan 1994
 */
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include "punix.h"
#include "globals.h"

#undef fflush

void seterrno(int e)
{
	errno = e;
}

/* el-cheapo buffered output */
int fflush(FILE *stream)
{
	char *b = P.user.charbuf;
	ssize_t s = P.user.charbufsize;
	while (s > 0) {
		ssize_t n = write(1, b, s);
		if (n < 0) break;
		b += n;
		s -= n;
	}
		
	P.user.charbufsize = 0;
	return 0;
}

STARTUP(int fputc(int c, FILE *stream))
{
	return -1; /* not implemented yet! */
}

STARTUP(int putchar(int c))
{
	P.user.charbuf[P.user.charbufsize++] = c;
	if (P.user.charbufsize >= 128 || c == '\n')
		fflush(NULL);
	return (unsigned char)c;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t m = nmemb;
	while (m) {
		ssize_t n;
		size_t s = size;

		while (s) {
			n = write(1, ptr, s);
			if (n < 0) goto out;
			ptr += n;
			s -= n;
		}
		--m;
	}
out:
	return nmemb - m;
}

typedef int (*vcbnprintf_callback_t)(int c, void *arg);
typedef int (*vcbscanf_get_callback_t)(void *arg);
typedef int (*vcbscanf_unget_callback_t)(int c, void *arg);

/*
 * vcbnprintf() is the heart of all *printf() routines
 * This takes a callback routine that is called once for every character that
 * is printed, up to 'size' characters. If 'size' is (size_t)-1, this is taken
 * as infinite. This routine returns the number of characters that would be
 * printed, regardless of the size.
 */
static int vcbnprintf(vcbnprintf_callback_t cb, void *arg,
                      size_t size, const char *fmt, va_list argp);

static int vcbscanf(vcbscanf_get_callback_t get, void *getarg,
                    vcbscanf_unget_callback_t unget, void *ungetarg,
	            const char *fmt, va_list argp);

#define PUT(c) do { \
	if (count++ < size || size == (size_t)-1) \
		put(c, cbarg); \
} while (0)

STARTUP(static int vcbnprintf(vcbnprintf_callback_t put, void *cbarg,
                              size_t size, const char *fmt, va_list argp))
{
	int c;
	enum { LEFT, RIGHT } adjust;
	enum { LONG, INT } intsize;
	int fill;
	int width, max, len;
	unsigned base;
	static const char X2C_tab[16] = "0123456789ABCDEF";
	static const char x2c_tab[16] = "0123456789abcdef";
	const char *x2c;
	char *p;
	long i;
	unsigned long u;
	char temp[8 * sizeof(long) / 3 + 2];
	size_t count = 0;
	
	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			/* Ordinary character. */
			PUT(c);
			continue;
		}
		
		/* Format specifier of the form:
		 *	%[adjust][fill][width][.max]keys
		 */
		c = *fmt++;
		
		adjust = RIGHT;
		if (c == '-') {
			adjust = LEFT;
			c = *fmt++;
		}
		
		fill = ' ';
		if (c == '0') {
			fill = '0';
			c = *fmt++;
		}
		
		width = 0;
		if (c == '*') {
			/* Width is specified as an argument, e.g. %*d. */
			width = va_arg(argp, int);
			c = *fmt++;
		} else if (isdigit(c)) {
			/* A number tells the width, e.g. %10d. */
			do {
				width = width * 10 + (c - '0');
			} while (isdigit(c = *fmt++));
		}
		
		max = INT_MAX;
		if (c == '.') {
			/* Max field length coming up. */
			if ((c = *fmt++) == '*') {
				max = va_arg(argp, int);
				c = *fmt++;
			} else if (isdigit(c)) {
				max = 0;
				do {
					max = max * 10 + (c - '0');
				} while (isdigit(c = *fmt++));
			}
		}
		
		/* Set a few flags to the default. */
		x2c = x2c_tab;
		i = 0;
		base = 10;
		intsize = INT;
		if (c == 'l' || c == 'j' || c == 't' || c == 'z') {
			/* "Long" key, e.g. %ld. */
			intsize = LONG;
			c = *fmt++;
		} else if (c == 'h') {
			c = *fmt++;
		}
		if (c == '\0') break;
		
		switch (c) {
		case 'd':
		case 'i':
			i = intsize == LONG
				? va_arg(argp, long)
				: va_arg(argp, int);
			u = i < 0 ? -i : i;
			goto int2ascii;
		
			/* pointer */
		case 'p':
			u = (unsigned long)va_arg(argp, void *);
			if (u) {
				/* leading zeroes */
				fill = '0';
				/* print at least six digits */
				width = width < 6 ? 6 : width;
				/* hexadecimal */
				base = 0x10;
				goto int2ascii;
			}
			/* our pointer is NULL. print "(nil)" as a string */
			p = "(nil)";
			len = 5;
			goto string_print;
			
			/* Octal. */
		case 'o':
			base = 010;
			goto getint;
			
			/* Hexadecimal.  %X prints upper case A-F, not %lx. */
		case 'X':
			x2c = X2C_tab;
		case 'x':
			base = 0x10;
			goto getint;
			
			/* Unsigned decimal. */
		case 'u':
		getint:
			u = intsize == LONG
				? va_arg(argp, unsigned long)
				: va_arg(argp, unsigned int);
		int2ascii:
			p = temp + sizeof(temp) - 1;
			*p = 0;
			do {
				*--p = x2c[u % base];
			} while ((u /= base) > 0);
			goto string_length;
			
			/* A character. */
		case 'c':
			p = temp;
			*p = (char)va_arg(argp, int);
			len = 1;
			goto string_print;
			
			/* Simply a percent. */
		case '%':
			p = temp;
			*p = '%';
			len = 1;
			goto string_print;
			
			/* A string.  The other cases will join in here. */
		case 's':
			p = va_arg(argp, char *);
			
		string_length:
			for (len = 0; p[len] != '\0' && len < max; ++len)
				;
			
		string_print:
			if (c == 's' || adjust == LEFT) fill = ' ';
			width -= len;
			if (i < 0) --width;
			if (fill == '0' && i < 0) PUT('-');
			if (adjust == RIGHT) {
				for (; width > 0; --width) PUT(fill);
			}
			if (fill == ' ' && i < 0) PUT('-');
			for (; len > 0; --len)
				PUT((unsigned char) *p++);
			for (; width > 0; --width)
				PUT(fill);
			break;
			
			/* Unrecognized format key, echo it back. */
		default:
			PUT('%');
			PUT(c);
		}
	}
	return count;
}

STARTUP(int fprintf(FILE *stream, const char *fmt, ...))
{
	int n;
	va_list argp;
	va_start(argp, fmt);
	
	n = vcbnprintf((vcbnprintf_callback_t)fputc, stream,
	               (size_t)-1, fmt, argp);
	va_end(argp);
	return n;
}

STARTUP(int printf(const char *fmt, ...))
{
	int n;
	va_list argp;
	va_start(argp, fmt);
	
	n = vcbnprintf((vcbnprintf_callback_t)putchar, NULL,
	               (size_t)-1, fmt, argp);
	fflush(NULL);
	va_end(argp);
	return n;
}

STARTUP(int sputc(int c, void *a))
{
	char **s = (char **)a;
	*(*s)++ = c;
	return (unsigned char)c;
}

STARTUP(int snprintf(char *s, size_t size, const char *fmt, ...))
{
	int n;
	va_list argp;
	va_start(argp, fmt);

	n = vcbnprintf(sputc, &s, size - 1, fmt, argp);
	*s = '\0';
	va_end(argp);
	return n;
}

STARTUP(int sprintf(char *s, const char *fmt, ...))
{
	int n;
	va_list argp;
	va_start(argp, fmt);

	n = vcbnprintf(sputc, &s, (size_t)-1, fmt, argp);
	*s = '\0';
	va_end(argp);
	return n;
}

STARTUP(int vfprintf(FILE *stream, const char *fmt, va_list argp))
{
	return vcbnprintf((vcbnprintf_callback_t)fputc, stream,
	                  (size_t)-1, fmt, argp);
}

STARTUP(int vprintf(const char *fmt, va_list argp))
{
	return vcbnprintf((vcbnprintf_callback_t)putchar, NULL,
	                  (size_t)-1, fmt, argp);
}

STARTUP(int vsnprintf(char *s, size_t size, const char *fmt, va_list argp))
{
	int n;
	n = vcbnprintf(sputc, s, size - 1, fmt, argp);
	*s = '\0';
	return n;
}

STARTUP(int vsprintf(char *s, const char *fmt, va_list argp))
{
	int n;
	n = vcbnprintf(sputc, s, (size_t)-1, fmt, argp);
	*s = '\0';
	return n;
}

/*
 * $PchHeader: /mount/hd2/minix/lib/minix/other/RCS/printk.c,v 1.2 1994/09/07 18:45:05 philip Exp $
 */
