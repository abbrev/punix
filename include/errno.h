#ifndef _ERRNO_H_
#define _ERRNO_H_

/* $Id$ */

/* not POSIX compliant - need to add some codes */

/* #include <sys/types.h> */

/* Error codes */		/*- if not used */
#define EPERM		1		/* Operation not permitted */
#define ENOENT		2		/* No such file or directory */
#define ESRCH		3	/*-*/	/* No such process */
#define EINTR		4		/* Interrupted system call */
#define EIO		5		/* I/O error */
#define ENXIO		6		/* No such device or address */
#define E2BIG		7		/* Arg list too long */
#define ENOEXEC 	8		/* Exec format error */
#define EBADF		9		/* Bad file number */
#define ECHILD		10		/* No child processes */
#define EAGAIN		11		/* Try again */
#define EWOULDBLOCK	EAGAIN		/* synonym */
#define ENOMEM		12		/* Out of memory */
#define EACCES		13		/* Permission denied */
#define EFAULT		14		/* Bad address */
#define ENOTBLK 	15		/* Block device required */
#define EBUSY		16		/* Device or resource busy */
#define EEXIST		17		/* File exists */
#define EXDEV		18		/* Cross-device link */
#define ENODEV		19		/* No such device */
#define ENOTDIR 	20		/* Not a directory */
#define EISDIR		21		/* Is a directory */
#define EINVAL		22		/* Invalid argument */
#define ENFILE		23		/* File table overflow */
#define EMFILE		24	/*-*/	/* Too many open files */
#define ENOTTY		25	/*-*/	/* Not a typewriter */
#define ETXTBSY 	26	/*-*/	/* Text file busy */
#define EFBIG		27	/*-*/	/* File too large */
#define ENOSPC		28		/* No space left on device */
#define ESPIPE		29		/* Illegal seek */
#define EROFS		30	/*-*/	/* Read-only file system */
#define EMLINK		31	/*-*/	/* Too many links */
#define EPIPE		32		/* Broken pipe */
#define EDOM		33	/*-*/	/* Math argument out of domain */
#define ERANGE		34	/*-*/	/* Math result not representable */
#define EDEADLK 	35	/*-*/	/* Resource deadlock would occur */
#define ENAMETOOLONG	36	/*-*/	/* File name too long */
#define ENOLCK		37	/*-*/	/* No record locks available */
#define ENOSYS	 	38		/* Syscall not implemented */
#define ENOTEMPTY	39	/*-*/	/* Directory not empty */
#define ELOOP		40	/*-*/	/* Too many symbolic links */

#define __ERRORS	40

/* FIXME: add the following error numbers to be POSIX-compliant:
 * EADDRINUSE
 * EADDRNOTAVAIL
 * EAFNOSUPPORT
 * EALREADY
 * EBADMSG
 * ECANCELED
 * ECONNABORTED
 * ECONNREFUSED
 * ECONNRESET
 * EDESTADDRREQ
 * EDQUOT
 * EHOSTUNREACH
 * EIDRM
 * EILSEQ
 * EINPROGRESS
 * EISCONN
 * EMSGSIZE
 * ENETDOWN
 * ENETRESET
 * ENETUNREACH
 * ENOBUFS
 * ENOTCONN
 * ENOTSOCK
 * ENOTSUP
 * ENXIO
 * EPROTO
 * EPROTONOSUPPORT
 * EPROTOTYPE
 * ESTALE
 */

#ifdef __KERNEL__
/* kernel-only error codes */
#define ERESTART	1024		/* restart syscall */
#define EJUSTRETURN	1025		/* just return */
#endif

/*
extern int sys_nerr;
extern char *sys_errlist[];

extern char *strerror(int _errno);
*/

extern int errno;
// this is a hack until we get proper support for data sections
#define errno (P.user.u_errno)

#endif /* _ERRNO_H_ */
