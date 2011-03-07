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

STARTUP(void sys_getdtablesize())
{
	P.p_retval = NOFILE;
}

struct rdwra {
	int fd;
	void *buf;
	size_t count;
};

STARTUP(static void rdwr(int mode))
{
	struct rdwra *ap = (struct rdwra *)P.p_arg;
	struct file *fp;
	
	GETF(fp, ap->fd);
	
	if ((fp->f_flag & mode) == 0) {
		P.p_error = EBADF;
		return;
	}
	P.p_base = ap->buf;
	P.p_count = ap->count;
	
	/* if we were interrupted by a signal, just return the byte count if
	 * it's not zero, else return an error */
	if (setjmp(P.p_sigjmp)) {
		if (P.p_count == ap->count) {
			P.p_error = EINTR;
			return;
		} else {
			goto out;
		}
	}
	
	if (fp->f_type == DTYPE_PIPE) {
		if (mode == FREAD)
			readp(fp);
		else
			writep(fp);
	} else {
		struct inode *ip = fp->f_inode;
		P.p_offset = fp->f_offset;
		
		if (!(ip->i_mode & IFCHR))
			i_lock(ip);
		
		rdwr_inode(ip, mode);
		
		if (!(ip->i_mode & IFCHR))
			i_unlock(ip);
		
		fp->f_offset += ap->count - P.p_count;
	}
out:
	P.p_retval = ap->count - P.p_count;
}

STARTUP(void sys_read(void))
{
	rdwr(FREAD);
}

#if 0
/* modified from 4.4BSD-Lite */
void sys_read(void)
{
	struct rdwr *ap = (struct rdwr *)P.p_arg;
	
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	long cnt;
	int error = 0;
	
	GETF(fp, ap->fd);
	if (!(fp->f_flag & FREAD)) {
		P.p_error = EBADF;
		return;
	}
	
	aiov.iov_base = ap->buf;
	aiov.iov_len = ap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = ap->count;
	auio.uio_rw = UIO_READ;
#if 0
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = current;
#endif
	cnt = ap->count;
	//error = f_read(fp, &auio);
	if (error = (*fp->f_ops->fo_read)(fp, &auio, fp->f_cred))
		if (auio.uio_resid != cnt && (error == ERESTART
		     || error == EINTR || error == EWOULDBLOCK))
			error = 0;
	cnt -= auio.uio_resid;
	*retval = cnt;
	return (error);
}

#endif

STARTUP(void sys_write())
{
	int whereami = G.whereami;
	G.whereami = WHEREAMI_WRITE;
	
	rdwr(FWRITE);
	
	G.whereami = whereami;
}

STARTUP(static void doopen(const char *pathname, int flags, mode_t mode))
{
	struct inode *ip;
	struct file *fp;
	int fd;
	int i;
	int inomode;
	
	fd = falloc();
	if (fd < 0)
		return;
	fp = P.p_ofile[fd];
	fp->f_flag = flags & O_RDWR;
	fp->f_type = DTYPE_INODE;
	mode = mode & 077777 & ~ISVTX;
	
	/* eliminate invalid flag combinations */
	
	inomode = INO_SOUGHT;
	if (flags & O_CREAT)
		inomode = INO_CREATE;
	
	/* ... */
	P.p_error = EMFILE;
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
	
	ip = &G.inode[G.nextinode]; /* XXX very hackish :) */
	if (!strcmp(ap->pathname, "/dev/vt"))
		ip->i_rdev = DEV_VT;
	else if (!strcmp(ap->pathname, "/dev/audio"))
		ip->i_rdev = DEV_AUDIO;
	else if (!strcmp(ap->pathname, "/dev/link"))
		ip->i_rdev = DEV_LINK;
	else if (!strcmp(ap->pathname, "/dev/null"))
		ip->i_rdev = DEV_MISC | 0;
	else if (!strcmp(ap->pathname, "/dev/zero"))
		ip->i_rdev = DEV_MISC | 1;
	else if (!strcmp(ap->pathname, "/dev/full"))
		ip->i_rdev = DEV_MISC | 2;
	else if (!strcmp(ap->pathname, "/dev/random"))
		ip->i_rdev = DEV_MISC | 3;
	else {
		P.p_error = ENOENT;
		return;
	}
	fd = falloc();
	if (fd < 0)
		return;
	++G.nextinode;
	cdevsw[MAJOR(ip->i_rdev)].d_open(ip->i_rdev,1);
	ip->i_mode = IFCHR;
	
	fp = P.p_ofile[fd];
	fp->f_flag = O_RDWR;
	fp->f_type = DTYPE_INODE;
	fp->f_inode = ip;
	fp->f_count = 1;
	
	P.p_retval = fd;
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

/* Internal close routine. Decrement reference count on file structure
 * and call special file close routines on last close. */
STARTUP(static void closef(struct file *fp))
{
        struct inode *inop;
        dev_t dev;
        int major;
        int rw;
        
	if (--fp->f_count > 0) return;
	
	inop = fp->f_inode;
	dev = inop->i_rdev;
	
	i_lock(inop);
	if (fp->f_type == DTYPE_PIPE) {
		inop->i_mode &= ~(IREAD | IWRITE);
		wakeup(inop); /* XXX */
	}
	i_unref(inop);
	
	switch (inop->i_mode & IFMT) {
	case IFCHR:
		cdevsw[MAJOR(dev)].d_close(dev, fp->f_flag);
		break;
	case IFBLK:
		bdevsw[MAJOR(dev)].d_close(dev, fp->f_flag);
	}
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
	
	closef(fp);
}

STARTUP(static void dupit(int newfd, struct file *oldfp, int flags))
{
	P.p_ofile[newfd] = oldfp;
	P.p_oflag[newfd] = flags;
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
	
	dupit(newfd, oldfp, P.p_oflag[ap->oldfd] & ~FD_CLOEXEC);
}

STARTUP(void sys_dup2())
{
	struct dupa *ap = (struct dupa *)P.p_arg;
	struct file *oldfp;
	
	GETF(oldfp, ap->oldfd);
	
	/* if they're the same fd, just return */
	if (ap->newfd == ap->oldfd)
		return;
	
	if ((unsigned)ap->newfd >= NOFILE) {
		P.p_error = EBADF;
		return;
	}
	
	if (P.p_ofile[ap->newfd])
		closef(P.p_ofile[ap->newfd]);
	
	P.p_error = 0;
	
	dupit(ap->newfd, oldfp, P.p_oflag[ap->oldfd] & ~FD_CLOEXEC);
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
	
	if (fp->f_type == DTYPE_PIPE) {
		P.p_error = ESPIPE;
		return;
	}
	
	switch (ap->whence) {
		case SEEK_SET:
			pos = ap->offset;
			break;
		case SEEK_CUR:
			pos = ap->offset + fp->f_offset;
			break;
		case SEEK_END:
			pos = ap->offset + fp->f_inode->i_size;
			break;
		default:
			P.p_error = EINVAL;
			return;
	}
	fp->f_offset = pos;
	
	P.p_retval = (long)pos;
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
	char *ofp;
	
	GETF(fp, ap->fd);
	ofp = &P.p_oflag[ap->fd];
	switch (ap->cmd) {
		case F_DUPFD:
			arg = ap->arg;
			if (arg < 0 || arg >= NOFILE) {
				P.p_error = EINVAL;
				return;
			}
			if ((arg = fdalloc(arg)) < 0)
				return;
			dupit(arg, fp, *ofp & ~FD_CLOEXEC);
			break;
		case F_GETFD:
			P.p_retval = *ofp & FD_CLOEXEC;
			break;
		case F_SETFD:
			*ofp = (*ofp & ~FD_CLOEXEC) | (ap->arg & FD_CLOEXEC);
			break;
		case F_GETFL:
#define OFLAGS(x) ((x) - 1) /* FIXME: what is OFLAGS, really? */
			P.p_retval = OFLAGS(fp->f_flag);
			break;
#if 0
		case F_SETFL:
			/* FIXME: define all these macros */
			fp->f_flag &= ~FCNTLFLAGS;
			fp->f_flag |= (FFLAGS(ap->arg)) & FCNTLFLAGS;
			if ((P.p_error = fset(fp, FNONBLOCK, fp->f_flag & NONBLOCK)))
				break;
			if ((P.p_error = fset(fp, O_ASYNC, fp->f_flag & O_ASYNC)))
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
	int dev;
	
	GETF(fp, ap->d);
	dev = fp->f_inode->i_rdev;
	
	cdevsw[MAJOR(dev)].d_ioctl(dev, ap->request, ap->arg);
}

