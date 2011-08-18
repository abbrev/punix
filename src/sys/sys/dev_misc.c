/*
 * dev_misc.c, miscellaneous system devices
 * Copyright 2008 Christopher Williams
 * 
 * $Id$
 * 
 * This implements /dev/null, /dev/zero, /dev/full, and /dev/random.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* device minor values */
#define DEVNULL   0
#define DEVZERO   1
#define DEVFULL   2
#define DEVRANDOM 3
#define DEVLAST   3

/*
 * a simple linear congruential pseudo-random number generator
 * Note: this generator is not cryptographically secure. Do not use it for
 * encryption!
 */
STARTUP(int rand())
{
	G.prngseed = 1103515245UL*G.prngseed + 12345UL;
	return ((unsigned)(G.prngseed / 65536) % 32768);
}

STARTUP(void srand(unsigned s))
{
	G.prngseed = s;
}

int misc_open(struct file *fp, struct inode *ip)
{
	int minor = MINOR(ip->i_rdev);
	
	if (minor > DEVLAST) {
		P.p_error = ENXIO;
		return -1;
	}

	return 0;
}

int misc_close(struct file *fp)
{
	return 0;
}

ssize_t misc_read(struct file *fp, void *buf, size_t count, off_t *pos)
{
	int minor = MINOR(fp->f_inode->i_rdev);
	size_t n;

	if (badbuffer(buf, count)) {
		P.p_error = EFAULT;
		return -1;
	}

	switch (minor) {
	case DEVNULL:
		return 0;
	case DEVZERO:
	case DEVFULL:
		memset(buf, 0, count);
		break;
	case DEVRANDOM:
		n = count;
		while (n--)
			*(char *)buf++ = (rand() >> 7);
	}
	return count;
}

ssize_t misc_write(struct file *fp, void *buf, size_t count, off_t *pos)
{
	int minor = MINOR(fp->f_inode->i_rdev);
	size_t n;

	if (badbuffer(buf, count)) {
		P.p_error = EFAULT;
		return -1;
	}

	switch (minor) {
	case DEVFULL:
		P.p_error = ENOSPC;
		return -1;
	case DEVRANDOM:
		/* stir the pot */
		n = count;
		while (n--)
			G.prngseed = G.prngseed *
			  (*(unsigned char *)buf++ + 1103515245UL) + 12345UL;
	}
	return count;
}

int misc_ioctl(struct file *fp, int request, void *arg)
{
	P.p_error = EINVAL;
	return -1;
}

off_t pipe_lseek(struct file *, off_t, int);
int generic_file_fstat(struct file *fp, struct stat *buf);
const struct fileops misc_fileops = {
	.open = misc_open,
	.close = misc_close,
	.read = misc_read,
	.write = misc_write,
	.lseek = pipe_lseek,
	.ioctl = misc_ioctl,
	.fstat = generic_file_fstat,
};

