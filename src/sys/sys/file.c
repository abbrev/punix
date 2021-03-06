/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
 * 
 * $Id: file.c 354 2011-03-16 08:13:32Z fredfoobar $
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* these are common file routines and generic file operation routines */

#include <limits.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "proc.h"
#include "file.h"
#include "buf.h"
#include "dev.h"
#include "punix.h"
#include "process.h"
/*
#include "uio.h"
*/
#include "inode.h"
#include "queue.h"
#include "globals.h"

int generic_file_open(struct file *fp, struct inode *ip)
{
	P.p_error = ENOSYS;
	return -1;
}

int generic_file_close(struct file *fp)
{
	P.p_error = ENOSYS;
	return -1;
}

ssize_t generic_file_read(struct file *fp, void *buf, size_t count, off_t *pos)
{
	return 0;
}

ssize_t generic_file_write(struct file *fp, void *buf, size_t count, off_t *pos)
{
	return 0;
}

off_t generic_file_lseek(struct file *fp, off_t offset, int whence)
{
	off_t base, newoffset;
	
	switch (whence) {
		case SEEK_SET:
			base = 0;
			break;
		case SEEK_CUR:
			base = fp->f_offset;
			break;
		case SEEK_END:
			base = fp->f_inode->i_size;
			break;
		default:
			P.p_error = EINVAL;
			return -1;
	}
	newoffset = base + offset;
	if (newoffset < 0) {
		/* if base and offset are both positive,
		 * newoffset is negative due to overflow */
		if (base > 0 && offset > 0)
			P.p_error = EOVERFLOW;
		else
			P.p_error = EINVAL;

		return -1;
	}

	fp->f_offset = newoffset;
	
	return newoffset;
}

int generic_file_fstat(struct file *fp, struct stat *buf)
{
	P.p_error = ENOSYS;
	return -1;
}

int generic_file_ioctl(struct file *fp, int request, ...)
{
	P.p_error = ENOSYS;
	return -1;
}

const struct fileops generic_file_fileops = {
	.open = generic_file_open,
	.close = generic_file_close,
	.read = generic_file_read,
	.write = generic_file_write,
	.lseek = generic_file_lseek,
	.ioctl = generic_file_ioctl,
	.fstat = generic_file_fstat,
};

int generic_special_open(struct file *fp, struct inode *ip)
{
	int major = MAJOR(ip->i_rdev);

	if (S_ISCHR(ip->i_mode)) {
		ip->i_fops = cdevsw[major];
	} else if (S_ISBLK(ip->i_mode)) {
		ip->i_fops = bdevsw[major].fileops;
	} else {
		abort();
	}
	fp->f_ops = ip->i_fops;
	assert(fp->f_ops);
	assert(fp->f_ops->open);
	return fp->f_ops->open(fp, ip);
}

const struct fileops generic_special_fileops = {
	.open = generic_special_open,
};

/*
 * Internal routine to close a file.
 * Decrement the reference count.
 * On the last reference, also put the inode back.
 */
int closef(struct file *fp)
{
	int ret = fp->f_ops->close(fp);
	if (--fp->f_count == 0)
		iput(fp->f_inode);
	return ret;
}

/* allocate a file descriptor */
/* start searching at 'd' */
STARTUP(int fdalloc(int d))
{
	/* look for lowest free fd */
	for (; d < NOFILE; ++d) {
		if (P.p_ofile[d] == NULL) {
			P.p_retval = d;
			//P.p_oflag[d] = 0;
			fdflags_to_oflag(d, 0L);
#if 0
			if (d > P.p_lastfile)
				P.p_lastfile = d;
#endif
			return d;
		}
	}
	
	P.p_error = EMFILE;
	return -1;
}

/* allocate a file descriptor and a file structure */
/* NOTE! this returns the fd instead of the fp as in V6 */
STARTUP(int falloc())
{
	struct file *fp;
	int fd;
	
	if ((fd = fdalloc(0)) < 0)
		return -1;
	
	for EACHFILE(fp) {
		if (fp->f_count == 0) {
			P.p_ofile[fd] = fp;
			++fp->f_count;
			fp->f_offset = 0;
			return fd;
		}
	}
	kprintf("no free files!\n");
	P.p_error = ENFILE;
	return -1;
}
