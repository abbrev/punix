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

// XXX old method
STARTUP(void miscopen(dev_t dev, int rw))
{
	int minor = MINOR(dev);
	
	if (minor > DEVLAST) {
		P.p_error = ENXIO;
		return;
	}
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

// XXX old method
STARTUP(void miscread(dev_t dev))
{
	int minor = MINOR(dev);
	
	switch (minor) {
	case DEVZERO:
	case DEVFULL:
		if (!P.p_base) {
			P.p_error = EFAULT;
			return;
		}
		memset(P.p_base, 0, P.p_count);
		P.p_count = 0;
		break;
	case DEVRANDOM:
		while (passc((unsigned char)(rand() >> 7)) >= 0) /* XXX constant */
			;
		break;
	}
}

ssize_t misc_read(struct file *fp, void *buf, size_t count)
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

// XXX old method
STARTUP(void miscwrite(dev_t dev))
{
	int minor = MINOR(dev);
	int c;
	
	switch (minor) {
	case DEVZERO:
	case DEVNULL:
		P.p_count = 0;
		break;
	case DEVFULL:
		P.p_error = ENOSPC;
		break;
	case DEVRANDOM:
		/* stir the pot */
		while ((c = cpass()) >= 0)
			G.prngseed = G.prngseed * (c + 1103515245UL) + 12345UL;
	}
}

ssize_t misc_write(struct file *fp, void *buf, size_t count)
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

// XXX old method
STARTUP(void miscioctl(dev_t dev, int cmd, void *cmargs))
{
}

int misc_ioctl(struct file *fp, int request, void *arg)
{
	P.p_error = EINVAL;
	return -1;
}

off_t pipe_lseek(struct file *, off_t, int);
const struct fileops misc_fileops = {
	.open = misc_open,
	.close = misc_close,
	.read = misc_read,
	.write = misc_write,
	.lseek = pipe_lseek,
	.ioctl = misc_ioctl,
};

#if 0 /* for reference */
struct fileops {
	int (*open)(struct file *, struct inode *);
	int (*close)(struct file *);
	ssize_t (*read)(struct file *, void *, size_t);
	ssize_t (*write)(struct file *, void *, size_t);
	off_t (*lseek)(struct file *, off_t, int);
	int (*ioctl)(struct file *, int, void *);
};
#endif
