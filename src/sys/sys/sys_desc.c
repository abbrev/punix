/*
 * Punix, Puny Unix kernel
 * Copyright 2005-2008 Christopher Williams
 * 
 * $Id$
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

/*-
 * Copyright (c) 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* everything in here manipulates file descriptors */

#include <limits.h>
#include <unistd.h>
#include <stddef.h>
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
#include "fs.h"
#include "queue.h"
#include "globals.h"

STARTUP(void sys_getdtablesize())
{
	P.p_retval = NOFILE;
}

struct prdwra {
	int fd;
	void *buf;
	size_t count;
	off_t offset;
};

/* values for whichoffset */
enum {
	OFFSET_FILE, /* use (and update) offset from file structure */
	OFFSET_ARG, /* use offset from the syscall argument (pread/pwrite) */
};

STARTUP(static void rdwr(int mode, int whichoffset))
{
	struct prdwra *ap = (struct prdwra *)P.p_arg;
	struct file *fp;
	off_t *offset = NULL;
	
	GETF(fp, ap->fd);
	
	if ((fp->f_flags & mode) == 0) {
		P.p_error = EBADF;
		return;
	}
	P.p_base = ap->buf;
	P.p_count = ap->count;
	ssize_t n;
	
	/* if we were interrupted by a signal, just return the byte count if
	 * it's not zero, else return an error */
	/*
	 * XXX: this only works right if the read/write routine updates
	 * P.p_count between interruptible sleeps. If the routine doesn't
	 * update P.p_count, it needs to catch signals itself and return the
	 * count or error condition, as we do here.
	 */
	if (setjmp(P.p_sigjmp)) {
		n = ap->count - P.p_count;
		if (n == 0) {
			//P.p_error = EINTR;
			return;
		} else {
			P.p_error = 0;
			goto out;
		}
	}
	
	offset = (whichoffset == OFFSET_FILE) ? &fp->f_offset : &ap->offset;
	if (*offset < 0) {
		P.p_error = EINVAL;
		return;
	}

	assert(fp->f_ops);
	if (mode == FREAD) {
		assert(fp->f_ops->read);
		n = fp->f_ops->read(fp, ap->buf, ap->count, offset);
	} else {
		assert(fp->f_ops->write);
#if 0
		kprintf("%s (%d): fp->f_ops->write=%p\n",
		        __FILE__, __LINE__, fp->f_ops->write);
#endif
		n = fp->f_ops->write(fp, ap->buf, ap->count, offset);
	}

out:
	P.p_retval = n;
}

STARTUP(void sys_read(void))
{
	rdwr(FREAD, OFFSET_FILE);
}

STARTUP(void sys_write())
{
	int whereami = G.whereami;
	G.whereami = WHEREAMI_WRITE;
	
	rdwr(FWRITE, OFFSET_FILE);
	
	G.whereami = whereami;
}

STARTUP(void sys_pread(void))
{
	rdwr(FREAD, OFFSET_ARG);
}

STARTUP(void sys_pwrite(void))
{
	rdwr(FWRITE, OFFSET_ARG);
}

STARTUP(static void doopen(const char *pathname, int flags, mode_t mode))
{
	struct inode *ip;
	struct file *fp;
	int fd;
	int i;
	int inomode;
	
#if 0
	fd = falloc();
	if (fd < 0)
		return;
	fp = P.p_ofile[fd];
	fp->f_flags = flags & O_RDWR;
	fp->f_type = DTYPE_INODE;
	mode = mode & 077777 & ~S_ISVTX;
	
	/* eliminate invalid flag combinations */
	
	inomode = INO_SOUGHT;
	if (flags & O_CREAT)
		inomode = INO_CREATE;
	
	/* ... */
#endif
	P.p_error = EMFILE;
}

extern const struct fileops generic_file_fileops;
extern const struct fileops generic_dir_fileops;
extern const struct fileops generic_special_fileops;

int fakefs_read_inode(struct inode *ip)
{
	return 0;
}

/*
 * calling this namei0() because it's just a temporary placeholder for a real
 * namei() function
 */
struct inode *namei0(const char *path)
{
	struct inode *ip;
	const struct filesystem *fs = NULL;
	dev_t dev;
	ino_t inum;
	struct file_list { const char *name; ino_t inum; dev_t dev; };
	static const struct file_list file_list[] = {
		{ "/dev/vt", 1, DEV_VT },
		{ "/dev/audio", 2, DEV_AUDIO },
		{ "/dev/link", 3, DEV_LINK },
		{ "/dev/null", 4, DEV_MISC|0 },
		{ "/dev/zero", 5, DEV_MISC|1 },
		{ "/dev/full", 6, DEV_MISC|2 },
		{ "/dev/random", 7, DEV_MISC|3 },
		{ NULL, 0, 0 },
	};
	static const struct fsops fakefsops = {
		.read_inode = fakefs_read_inode
	};
	static const struct filesystem fakefs = {
		.fsops = &fakefsops
	};

	const struct file_list *flp;
	
	for (flp = &file_list[0]; flp->name; ++flp) {
		if (!strcmp(path, flp->name)) {
			fs = &fakefs;
			inum = flp->inum;
			dev = flp->dev;
			break;
		}
	}
	if (!fs) {
		P.p_error = ENOENT;
		return NULL;
	}

	ip = iget(fs, inum);
	if (!ip) goto fail;
	ip->i_rdev = dev;
	ip->i_mode = S_IFCHR;

	// XXX This should be set in each filesystem's read_inode() handler.
	// The handler tables for regular files and directories would be
	// defined for each file system too. The special file handler table
	// would just be generic_special_fileops.
	if (S_ISREG(ip->i_mode)) {
		ip->i_fops = &generic_file_fileops;
	} else if (S_ISDIR(ip->i_mode)) {
		ip->i_fops = NULL; //&generic_dir_fileops;
	} else {
		ip->i_fops = &generic_special_fileops;
	}

	return ip;
fail:
	return NULL;
}

/* open system call */
/* FIXME: open is currently just a hack to produce results */
STARTUP(void sys_open())
{
	struct a {
		const char *pathname;
		int flags;
		mode_t mode;
	} *ap = (struct a *)P.p_arg;

#if 1	
	int fd;
	struct file *fp;
	struct inode *ip;
	
	fd = falloc();
	if (fd < 0)
		return;
	
	fp = P.p_ofile[fd];
	ip = namei0(ap->pathname);
	if (!ip) {
		if (ap->flags & O_CREAT) {
			P.p_error = EACCES; // XXX no way to create files yet
		}
		goto fail_file;
	}
	
	fp->f_flags = ap->flags; // XXX: validate flags
	fp->f_type = DTYPE_INODE;
	fp->f_inode = ip;
	if (ip->i_fops->open(fp, ip)) {
		goto fail_inode;
	}
	assert(fp->f_ops);
	assert(fp->f_ops->read);

	iunlock(ip);
	
	P.p_retval = fd;
	return;
fail_inode:
	iunlock(ip);
	iput(ip);
fail_file:
	--fp->f_count; // XXX free the file structure
	P.p_ofile[fd] = NULL;
	;
#else
	doopen(ap->pathname, ap->flags, ap->mode);
#endif
}

/* creat system call */
STARTUP(void sys_creat())
{
	struct a {
		const char *pathname;
		mode_t mode;
	} *ap = (struct a *)P.p_arg;
	
	doopen(ap->pathname, O_CREAT | O_TRUNC | O_WRONLY, ap->mode);
}

STARTUP(void sys_close())
{
	struct a {
		int fd;
	} *ap = (struct a *)P.p_arg;
	struct file *fp;
	
	GETF(fp, ap->fd);
	P.p_ofile[ap->fd] = NULL;
	
#if 0
	while (P.p_lastfile >= 0 && P.p_ofile[P.p_lastfile] == NULL)
		--P.p_lastfile;
#endif
	
	P.p_retval = closef(fp);
}

#define OFLAGIDX(num) ((num) >> 3) /* num/8 */
#define OFLAGMASK(num) ((num) & 7) /* num%8 */
#define SETBIT(x, bit) ((x) |= (1<<(bit)))
#define CLRBIT(x, bit) ((x) &= ~(1<<(bit)))
#define EQUBIT(x, bit, v) ((x) = ((x) & ~(1<<(bit)) | (1<<!!(v))))
#define SETMASK(x, mask) ((x) |= (mask))
#define CLRMASK(x, mask) ((x) &= ~(mask))
#define EQUMASK(x, mask, v) ((x) = ((x) & ~(mask)) | (1<<!!(v)))
#define GETMASK(x, mask) ((x) & (mask))

/*
 * convert an fdflags bitmap to the internal oflag structure for the given fd
 */
void fdflags_to_oflag(int fd, int fdflags)
{
	int idx = OFLAGIDX(fd);
	unsigned char mask = OFLAGMASK(fd);
	EQUMASK(current->p_oflag.cloexec[idx], mask, fdflags & FD_CLOEXEC);
}

/*
 * convert the internal oflag structure to an fdflags bitmap for the given fd
 */
long oflag_to_fdflags(int fd)
{
	int idx = OFLAGIDX(fd);
	unsigned char mask = OFLAGMASK(fd);
	long fdflags = 0;
	if (current->p_oflag.cloexec[idx] & mask)
		fdflags |= FD_CLOEXEC;
	/* add more here if we add more fd flags */
	return fdflags;
}

/*
 * clear the given fdflags (bitmap) in the internal oflag structure for the
 * given fd
 */
void clear_fdflags(int fd, int fdflags)
{
	int idx = OFLAGIDX(fd);
	unsigned char mask = OFLAGMASK(fd);
	if (fdflags & FD_CLOEXEC) {
		CLRMASK(current->p_oflag.cloexec[idx], mask);
	}
}

/*
 * set the given fdflags (bitmap) in the internal oflag structure for the
 * given fd
 */
void set_fdflags(int fd, int fdflags)
{
	int idx = OFLAGIDX(fd);
	unsigned char mask = OFLAGMASK(fd);
	if (fdflags & FD_CLOEXEC) {
		SETMASK(current->p_oflag.cloexec[idx], mask);
	}
}

STARTUP(static void dupit(int newfd, struct file *oldfp, int flags))
{
	P.p_ofile[newfd] = oldfp;
	fdflags_to_oflag(newfd, flags);
	++oldfp->f_count;
	
	P.p_retval = newfd;
}

struct dupa {
	int oldfd;
	int newfd;
};

STARTUP(void sys_dup())
{
	struct dupa *ap = (struct dupa *)P.p_arg;
	int newfd;
	struct file *oldfp;
	
	GETF(oldfp, ap->oldfd);
	
	if ((newfd = fdalloc(0)) < 0)
		return;
	
	dupit(newfd, oldfp, oflag_to_fdflags(ap->oldfd) & ~FD_CLOEXEC);
}

STARTUP(void sys_dup2())
{
	struct dupa *ap = (struct dupa *)P.p_arg;
	struct file *oldfp, *fp;
	
	GETF(oldfp, ap->oldfd);
	
	/* if they're the same fd, just return */
	if (ap->newfd == ap->oldfd)
		return;
	
	if ((unsigned)ap->newfd >= NOFILE) {
		P.p_error = EBADF;
		return;
	}
	
	fp = P.p_ofile[ap->newfd];
	if (fp)
		closef(fp);
	
	P.p_error = 0;
	
	dupit(ap->newfd, oldfp, oflag_to_fdflags(ap->oldfd) & ~FD_CLOEXEC);
}

STARTUP(void sys_lseek())
{
	struct a {
		int fd;
		off_t offset;
		int whence;
	} *ap = (struct a *)P.p_arg;
	off_t pos;
	struct file *fp;
	
	GETF(fp, ap->fd);
	
	P.p_retval = fp->f_ops->lseek(fp, ap->offset, ap->whence);
}

/* umask system call */
STARTUP(void sys_umask())
{
	struct a {
		mode_t mask;
	} *ap = (struct a *)P.p_arg;
	
	P.p_retval = P.p_cmask;
	P.p_cmask = ap->mask & 0777;
}

STARTUP(void sys_fstat())
{
	struct a {
		int fd;
		struct stat	*buf;
	} *ap = (struct a *)P.p_arg;
	struct stat buf;
	struct file *fp;
	
	GETF(fp, ap->fd);
	switch (fp->f_type) {
		case DTYPE_PIPE:
		case DTYPE_INODE:
#if 0
			P.p_error = ino_stat((struct inode *)fp->f_data, &buf);
			if (fp->f_type == DTYPE_PIPE)
				buf.st_size -= fp->f_offset;
			break;
#endif
		
		default:
			P.p_error = EINVAL;
			break;
	}
	if (!P.p_error)
		P.p_error = copyout(ap->buf, &buf, sizeof(buf));
}

#if 0
STARTUP(static int fioctl(struct file *fp, unsigned cmd, char *value))
{
	return Fops[fp->f_type]->fo_ioctl(fp, cmd, value);
}

STARTUP(static int fgetown(struct file *fp, uint32_t *valuep))
{
	int error;
	
	error = fioctl(fp, (unsigned)TIOCGPGRP, (char *)valuep);
	*valuep = -*valuep;
	return error;
}

STARTUP(static int fsetown(struct file *fp, uint32_t value))
{
	if (value > 0) {
		struct proc *p = pfind(value);
		if (!p)
			return ESRCH;
		value = p->p_pgrp;
	} else
		value = -value;
	return fioctl(fp, (unsigned)TIOCSPGRP, (char *)&value);
}
#endif

STARTUP(void sys_fcntl())
{
	struct a {
		int	fd;
		int	cmd;
		union {
			long	arg;
			struct flock	*lock;
		};
	} *ap = (struct a *)P.p_arg;
	struct file *fp;
	long arg;
	//char *ofp;
	long fdflags;
	
	GETF(fp, ap->fd);
	//ofp = &P.p_oflag[ap->fd];
	fdflags = oflag_to_fdflags(ap->fd);
	switch (ap->cmd) {
		case F_DUPFD:
			arg = ap->arg;
			if (arg < 0 || arg >= NOFILE) {
				P.p_error = EINVAL;
				return;
			}
			if ((arg = fdalloc(arg)) < 0)
				return;
			dupit(arg, fp, fdflags & ~FD_CLOEXEC);
			break;
		case F_GETFD:
			P.p_retval = fdflags & FD_CLOEXEC;
			break;
		case F_SETFD:
			fdflags = (fdflags & ~FD_CLOEXEC) | (ap->arg & FD_CLOEXEC);
			fdflags_to_oflag(ap->fd, fdflags);
			break;
		case F_GETFL:
#define OFLAGS(x) ((x) - 1) /* FIXME: what is OFLAGS, really? */
			P.p_retval = OFLAGS(fp->f_flags);
			break;
#if 0
		case F_SETFL:
			/* FIXME: define all these macros */
			fp->f_flags &= ~FCNTLFLAGS;
			fp->f_flags |= (FFLAGS(ap->arg)) & FCNTLFLAGS;
			if ((P.p_error = fset(fp, FNONBLOCK, fp->f_flags & NONBLOCK)))
				break;
			if ((P.p_error = fset(fp, O_ASYNC, fp->f_flags & O_ASYNC)))
				fset(fp, FNONBLOCK, 0);
			break;
#endif
#if 0
		case F_GETOWN:
			P.p_error = fgetown(fp, &P.p_retval);
			break;
		case F_SETOWN:
			P.p_error = fsetown(fp, ap->arg);
			break;
#endif
		default:
			P.p_error = EINVAL;
	}
}

STARTUP(void sys_ioctl())
{
	struct a {
		int d;
		int request;
		void *arg;
	} *ap = (struct a *)P.p_arg;
	
	int fd;
	struct file *fp;
	int major;
	
	GETF(fp, ap->d);
	major = MAJOR(fp->f_inode->i_rdev);

	P.p_retval = fp->f_ops->ioctl(fp, ap->request, ap->arg);
}

