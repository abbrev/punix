/*	printf() - printf() for user program		Author: Kees J. Bot
 *								15 Jan 1994
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include "punix.h"
#include "globals.h"

#undef fflush

int *_geterrnop()
{
	return &P.user.u_errno;
}

FILE **_getstream(int n)
{
	return (FILE **)&P.user.streams[n];
}

#undef errno
#define errno (P.user.u_errno)

void seterrno(int e)
{
	errno = e;
}

// bit flags for FILE_t.flags
enum {
	FILE_EOF     = 0x0001,
	FILE_ERROR   = 0x0002,
	FILE_READ    = 0x0004,
	FILE_WRITE   = 0x0008,
	FILE_TRUNC   = 0x0010,
	FILE_APPEND  = 0x0020,
	FILE_WRITING = 0x0040, // set if buffer contains unflushed write data
	FILE_BUF     = 0x0080, // buffered
	FILE_LBUF    = 0x0100, // line buffered (FILE_BUF must be set too)
};

#define FILE_BUF_SIZE 64
struct FILE_t {
	int fd;
	int flags;
	char buf[FILE_BUF_SIZE];
	int bufsize;
};

static FILE *newFILE()
{
	FILE *f = malloc(sizeof(FILE));
	if (!f) {
		return NULL;
	}
	f->bufsize = 0;
	f->flags = 0;
	return f;
}

/*
 * get file flags and (if oflagp is non-NULL) open's oflag
 * return flags, or set errno and return 0 on error
 */
static int getflags(const char *mode, int *oflagp)
{
	const char *p;
	int oflag = 0;
	int flags = 0;
	int rw = 0;
	for (p = mode; *p != '\0'; ++p) {
		switch (*p) {
		case 'r':
			if (flags & (FILE_READ|FILE_WRITE)) {
				errno = EINVAL; return 0;
			}
			flags |= FILE_READ;
			rw = O_RDONLY;
			break;
		case 'w':
			if (flags & (FILE_READ|FILE_WRITE)) {
				errno = EINVAL; return 0;
			}
			flags |= FILE_WRITE|FILE_TRUNC;
			oflag = O_CREAT;
			rw = O_WRONLY;
			break;
		case 'a':
			if (flags & (FILE_READ|FILE_WRITE)) {
				errno = EINVAL; return 0;
			}
			flags |= FILE_WRITE|FILE_APPEND;
			oflag = O_APPEND|O_CREAT;
			rw = O_WRONLY;
		case '+':
			if (!(flags & (FILE_READ|FILE_WRITE))) {
				errno = EINVAL; return 0;
			}
			flags |= FILE_READ|FILE_WRITE;
			rw = O_RDWR;
			break;
		case 'b':
			if (!(flags & (FILE_READ|FILE_WRITE))) {
				errno = EINVAL; return 0;
			}
			break;
		}
	}
	if (!(flags & (FILE_READ|FILE_WRITE))) {
		errno = EINVAL; return 0;
	}
	oflag |= rw;
	if (oflagp) *oflagp = oflag;
	return flags;
}

/* el-cheapo buffered output */
static int _fflush(FILE *f)
{
	if (!f) {
		errno = EBADF;
		return -1;
	}
	char *b = f->buf;
	ssize_t s = f->bufsize;
	int err = 0;
	if (f->flags & FILE_WRITING) {
		while (s > 0) {
			ssize_t n = write(f->fd, b, s);
			if (n < 0) {
				err = -1;
				break;
			}
			b += n;
			s -= n;
		}
	}
	
	f->bufsize = 0;
	if (err) {
		f->flags |= FILE_ERROR;
	}
	return err;
}

int fflush(FILE *f)
{
	if (f) {
		return _fflush(f);
	} else {
		// TODO: flush all streams
		_fflush(stdout);
		_fflush(stderr);
	}
	return 0;
}

static FILE *_fdopen(int fd, int flags)
{
	FILE *file = newFILE();
	if (!file) return NULL;
	file->fd = fd;
	file->flags = flags;
	return file;
}

FILE *fdopen(int fd, const char *mode)
{
	int flags = getflags(mode, NULL);
	if (!flags) {
		return NULL;
	}
	return _fdopen(fd, flags);
}

FILE *fopen(const char *name, const char *mode)
{
	FILE *f;
	int fd;
	int flags = 0;
	int oflag = 0;

	flags = getflags(mode, &oflag);
	if (!flags) goto error;

	fd = open(name, oflag, 0666);
	if (fd < 0) goto error;

	f = _fdopen(fd, flags);
	if (!f) goto close;

	return f;

close:
	close(fd);
error:
	return NULL;
}

int fclose(FILE *f)
{
	if (_fflush(f)) return -1;
	close(f->fd);
	free(f);
	return 0;
}

int fputc(int c, FILE *f)
{
	if (!f || !(f->flags & FILE_WRITE)) {
		errno = EBADF;
		return EOF;
	}
	if (!(f->flags & FILE_WRITING)) {
		// switch to writing mode
		_fflush(f);
		f->flags |= FILE_WRITING;
	}
	f->buf[f->bufsize++] = c;
	if (!(f->flags & FILE_BUF) ||
	    f->bufsize >= sizeof(f->buf) ||
	    ((f->flags & FILE_LBUF) && c == '\n'))
		_fflush(f);
	return (unsigned char)c;
}

#undef putc
int putc(int c, FILE *f)
{
	return fputc(c, f);
}

#undef putchar
int putchar(int c)
{
	return fputc(c, stdout);
}

int fgetc(FILE *f)
{
	if (!f || !(f->flags & FILE_READ)) {
		errno = EBADF;
		return EOF;
	}
	if (feof(f))
		return EOF;
	/*
	 * this shouldn't be needed (a conforming application will call fflush
	 * or fseek before doing input after doing output
	 */
	if (f->flags & FILE_WRITING) {
		_fflush(f);
		f->flags &= ~FILE_WRITING;
	}
	if (f == stdin) {
		_fflush(stdout);
		_fflush(stderr);
	}
	if (f->bufsize == 0) {
		// read up to half of our buffer size to leave room for ungetc
		ssize_t n = read(f->fd, f->buf, sizeof(f->buf)/2);
		if (n == 0)
			f->flags |= FILE_EOF;
		if (n < 0)
			f->flags |= FILE_ERROR;
		if (n <= 0)
			return EOF;
		f->bufsize = n;
	}
	int c = (unsigned char)f->buf[0];
	memmove(f->buf, f->buf+1, --f->bufsize);
	return c;
}

#undef getc
int getc(FILE *f)
{
	return fgetc(f);
}

#undef getchar
int getchar()
{
	return fgetc(stdin);
}

int ungetc(int c, FILE *f)
{
	if (c == EOF)
		return EOF;
	if (!f || !(f->flags & FILE_READ)) {
		errno = EBADF;
		return EOF;
	}
	if (f->flags & FILE_WRITING) {
		_fflush(f);
		f->flags &= ~FILE_WRITING;
	}
	if (f->bufsize >= sizeof(f->buf))
		return EOF; // XXX: what are we supposed to return?
	f->flags &= ~FILE_EOF;
	memmove(f->buf+1, f->buf, f->bufsize++);
	f->buf[0] = c;
	return (unsigned char)c;
}

int feof(FILE *f)
{
	if (!f) {
		errno = EBADF;
		return -1;
	}
	return (f->flags & FILE_EOF) != 0;
}

int ferror(FILE *f)
{
	if (!f) {
		errno = EBADF;
		return -1;
	}
	return (f->flags & FILE_ERROR) != 0;
}

void clearerr(FILE *f)
{
	if (!f) {
		errno = EBADF;
		return;
	}
	f->flags &= ~FILE_ERROR;
}

int fseek(FILE *f, long offset, int whence)
{
	if (_fflush(f)) return -1; // flush out write buffer
	off_t pos = lseek(f->fd, offset, whence);
	if (pos == (off_t)-1) {
		return -1;
	}
	return 0;
}

void rewind(FILE *f)
{
	fseek(f, 0, SEEK_SET);
}

int setvbuf(FILE *f, char *buf, int type, size_t size)
{
	switch (type) {
	case _IOFBF: f->flags |= FILE_BUF; f->flags &= ~FILE_LBUF; break;
	case _IOLBF: f->flags |= FILE_BUF|FILE_LBUF; break;
	case _IONBF: f->flags &= ~(FILE_BUF|FILE_LBUF); break;
	}
	// TODO: use buf and size if buf is not null
	return 0;
}

void setbuf(FILE *f, char *buf)
{
	setvbuf(f, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f)
{
	size_t m = nmemb;
	if (!f || !(f->flags & FILE_WRITE)) {
		errno = EBADF;
		return EOF;
	}
	// we're lazy; flush current buffer and call the write syscall directly
	_fflush(f);
	while (m) {
		ssize_t n;
		size_t s = size;

		while (s) {
			n = write(f->fd, ptr, s);
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

#if 0
static int vcbscanf(vcbscanf_get_callback_t get, void *getarg,
                    vcbscanf_unget_callback_t unget, void *ungetarg,
	            const char *fmt, va_list argp);
#endif

#define PUT(c) do { \
	if (count++ < size || size == (size_t)-1) \
		put(c, cbarg); \
} while (0)

static int vcbnprintf(vcbnprintf_callback_t put, void *cbarg,
                      size_t size, const char *fmt, va_list argp)
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
