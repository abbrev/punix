#ifndef _STDIO_H_
#define _STDIO_H_

/* $Id$ */

/* mostly POSIX compliant */

#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>

#define BUFSIZ	512	/* too big? */
#define _IOFBF		0x00	/* full buffering */
#define _IOLBF		0x01	/* line buffering */
#define _IONBF		0x02	/* no buffering */

/* define these
#define L_ctermid
#define L_tmpnam
*/

#define FILENAME_MAX	NAME_MAX
#define FOPEN_MAX	
#define TMP_MAX		

#define EOF	(-1)

#if 0
/* modified from /usr/include/libio.h: */
typedef struct {
	int flags;
	
	/* The following pointers correspond to the C++ streambuf protocol. */
	char	*read_ptr;	/* Current read pointer */
	char	*read_end;	/* End of get area. */
	char	*read_base;	/* Start of putback+get area. */
	char	*write_base;	/* Start of put area. */
	char	*write_ptr;	/* Current put pointer. */
	char	*write_end;	/* End of put area. */
	char	*buf_base;	/* Start of reserve area. */
	char	*buf_end;	/* End of reserve area. */
	/* The following fields are used to support backing up and undo. */
	char	ve_base;	/* Pointer to start of non-current get area. */
	char	ckup_base;	/* Pointer to first valid character of backup area */
	char	*save_end;	/* Pointer to end of non-current get area. */
	
	/*struct _IO_marker *_markers;*/
	
	struct FILE *chain;
	
	int	fileno;
	int	blksize;
	off_t	offset;
	
#if 0
#define __HAVE_COLUMN /* temporary */
	/* 1+column number of pbase(); 0 is unknown. */
	unsigned short _cur_column;
	signed char _vtable_offset;
	char _shortbuf[1];
	
	/*  char* _save_gptr;  char* _save_egptr; */
	
	_IO_lock_t *_lock;
#endif
} FILE;
#else
typedef struct FILE_t FILE;
#endif

extern FILE *stderr;
extern FILE *stdin;
extern FILE *stdout;

// XXX
FILE **_getstream(int);
#define stdin  (*_getstream(0))
#define stdout (*_getstream(1))
#define stderr (*_getstream(2))

#if 0
#define stdin	stdin
#define stdout	stdout
#define stderr	stderr
#endif

typedef off_t	fpos_t;

void	clearerr(FILE *__stream);
char	*ctermid(char *__s);
int	fclose(FILE *__stream);
FILE	*fdopen(int __fd, const char *__mode);
int	feof(FILE *__stream);
int	ferror(FILE *__stream);
int	fflush(FILE *__stream);
int	fgetc(FILE *__stream);
int	fgetpos(FILE *__stream, fpos_t *__pos);
char	*fgets(char *__s, int __size, FILE *__stream);
int	fileno(FILE *__stream);
FILE	*fopen(const char *__path, const char *__mode);
int	fprintf(FILE *__stream, const char *__format, ...);
int	fputc(int __c, FILE *__stream);
int	fputs(const char *__s, FILE *__stream);
size_t	fread(void *__buf, size_t __size, size_t __nmemb, FILE *__stream);
FILE	*freopen(const char *__path, const char *__mode, FILE *__stream);
int	fscanf(FILE *__stream, const char *__format, ...);
int	fseek(FILE *__stream, long __offset, int __whence);
int	fseeko(FILE *__stream, off_t __offset, int __whence);
int	fsetpos(FILE *__stream, const fpos_t *__pos);
long	ftell(FILE *__stream);
off_t	ftello(FILE *__stream);
size_t	fwrite(const void *__buf, size_t __size, size_t __nmemb,
		FILE *__stream);
int	getc(FILE *__stream);
#define getc(s) fgetc(s)
int	getchar(void);
#define getchar() fgetc(stdin)
char	*gets(char *__s);
int	pclose(FILE *__stream);
void	perror(const char *__s);
FILE	*popen(const char *__command, const char *__type);
int	printf(const char *__format, ...);
int	putc(int __c, FILE *__stream);
#define putc(c, s) fputc(c, s)
int	putchar(int __c);
#define putchar(c) fputc(c, stdout)
int	puts(const char *__s);
int	remove(const char *__path);
int	rename(const char *__oldpath, const char *__newpath);
void	rewind(FILE *__stream);
int	scanf(const char *__format, ...);
void	setbuf(FILE *__stream, char *__buf);
int	setvbuf(FILE *__stream, char *__buf, int __mode, size_t __size);
int	snprintf(char *__str, size_t __size, const char *__format, ...);
int	sprintf(char *__str, const char *__format, ...);
int	sscanf(const char *__str, const char *__format, ...);
char	*tempnam(const char *__dir, const char *__prefix);
FILE	*tmpfile(void);
char	*tmpnam(char *__s);
int	ungetc(int __c, FILE *__stream);
int	vfprintf(FILE *__stream, const char *__format, va_list __ap);
int	vfscanf(FILE *__stream, const char *__format, va_list __ap);
int	vprintf(const char *__format, va_list __ap);
int	vscanf(const char *__format, va_list __ap);
int	vsnprintf(char *__str, size_t __size, const char *__format,
		va_list __ap);
int	vsprintf(char *__str, const char *__format, va_list __ap);
int	vsscanf(const char *__str, const char *__format, va_list __ap);

#endif /* _STDIO_H_ */
