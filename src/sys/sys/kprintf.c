/*	kprintf() - kernel printf()			Author: Kees J. Bot
 *								15 Jan 1994
 */
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>

#include "punix.h"

/* kprintf() uses kputchar() to print characters. */
int kputchar(int c);

#define PUT(c) do { kputchar(c); ++count; } while (0)

STARTUP(int kprintf(const char *fmt, ...))
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
	
	va_list argp;
	
	va_start(argp, fmt);
	
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
			u = va_arg(argp, unsigned long);
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
			
		case 'z':
			intsize = LONG;
			/* fall through */
		
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
	
	va_end(argp);
	return count;
}

/*
 * $PchHeader: /mount/hd2/minix/lib/minix/other/RCS/printk.c,v 1.2 1994/09/07 18:45:05 philip Exp $
 */
