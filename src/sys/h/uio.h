#ifndef _UIO_H_
#define _UIO_H_

#include <sys/types.h>
#include <sys/uio.h>

enum uio_rw { UIO_READ, UIO_WRITE };

#if 0
/* Segment flag values. */
enum uio_seg {
	UIO_USERSPACE,		/* from user data space */
	UIO_SYSSPACE,		/* from system space */
	UIO_USERISPACE		/* from user I space */
};
#endif

struct uio {
	struct	iovec *uio_iov;
	int	uio_iovcnt;
	off_t	uio_offset;
	unsigned short	uio_resid;
#if 0
	enum	uio_seg uio_segflg;
#endif
	enum	uio_rw uio_rw;
};

#endif
