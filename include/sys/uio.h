#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_

/* should be POSIX compliant */

#include <sys/types.h>

struct iovec {
	void	*iov_base;	/* base address of memory region for input or output */
	size_t	iov_len;	/* size of the memory pointed to by iov_base */
};

ssize_t	readv(int __fd, const struct iovec *__vector, int __count);
ssize_t	writev(int __fd, const struct iovec *__vector, int __count);

#endif
