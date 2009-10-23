/*
 * Punix, Puny Unix kernel
 * Copyright 2007 Chris Williams
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

#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>

#include "proc.h"
#include "file.h"
#include "punix.h"
#include "process.h"
#include "fs.h"
#include "inode.h"
#include "buf.h"
#include "dev.h"
#include "queue.h"
#include "globals.h"

static void iomove(void *cp, int n, int flag)
{
        int t;
        
        if (n == 0)
                return;
        
        if (flag == B_WRITE)
                t = copyin(P.p_base, cp, n);
        else
                t = copyout(cp, P.p_base, n);
        
        if (t) {
                P.p_error = EFAULT;
                return;
        }
        P.p_base += n;
        P.p_offset += n;
        P.p_count -= n;
        return;
}

static void rdwr_inode(struct inode *inop, int mode)
{
	struct buf *bufp;
	dev_t dev;
	long lbn, bn;
	off_t diff;
	int on, n;
	int type;
	int bmode = mode == FREAD ? B_READ : B_WRITE;
	
	if (mode == FREAD && P.p_count == 0) return;
	if (P.p_offset < 0) {
		P.p_error = EINVAL;
		return;
	}
        dev = inop->i_rdev;
        type = inop->i_mode & IFMT;
	if (mode == FREAD) inop->i_flag != IACC;
	
        if (type == IFCHR) {
		if (mode == FWRITE) {
			inop->i_flag |= IUPD|ICHG;
			cdevsw[MAJOR(dev)].d_write(dev);
		} else {
			cdevsw[MAJOR(dev)].d_read(dev);
		}
                return;
        }
	if (mode == FWRITE && P.p_count == 0) return;
	
	do {
		lbn = bn = P.p_offset >> BLOCKSHIFT;
		on = P.p_offset & BLOCKMASK;
		n = MIN((unsigned)(BLOCKSIZE-on), P.p_count);
                if (type != IFBLK) {
			if (mode == FREAD) {
				diff = inop->i_size - P.p_offset;
				if (diff <= 0)
					return;
				if (diff < n)
					n = diff;
				bn = pfs_bmap(inop, bn, bmode);
				if (P.p_error)
					return;
			} else {
				bn = pfs_bmap(inop, bn, bmode);
				if ((long)bn < 0)
					return;
			}
			dev = inop->i_dev;
                }
		if (mode == FREAD) {
			if ((long)bn < 0) {
				bufp = blk_get(NODEV, 0);
				blk_clear(bufp);
			} else
				bufp = blk_read(dev, bn);
			n = MIN((unsigned)n, BLOCKSIZE-bufp->b_resid);
			if (mode == FWRITE || n)
				iomove(bufp->b_addr + on, n, bmode);
			blk_release(bufp);
			if (n <= 0) break;
		} else {
			if (n == BLOCKSIZE)
				bufp = blk_get(dev, bn);
			else
				bufp = blk_read(dev, bn);
			if (mode == FWRITE || n)
				iomove(bufp->b_addr + on, n, bmode);
			if (P.p_error)
				blk_release(bufp);
			else
				blk_write_delay(bufp);
			if (P.p_offset > inop->i_size && (type == IFDIR || type == IFREG))
				inop->i_size = P.p_offset;
			inop->i_flag |= IUPD|ICHG;
		}
	} while (!P.p_error && P.p_count != 0);
}

void read_inode(struct inode *inop)
{
	rdwr_inode(inop, FREAD);
}

void write_inode(struct inode *inop)
{
	rdwr_inode(inop, FWRITE);
}

#if 0 /* TAINTED */
STARTUP(struct file *getf(int fd))
{
	struct file *fp = NULL;
	
	if ((unsigned)fd < NOFILE)
		fp = P.p_ofile[fd];
	if (!fp)
		P.p_error = EBADF;
	
	return fp;
}

/* Internal close routine. Decrement reference count on file structure
 * and call special file close routines on last closef. */
STARTUP(void closef(struct file *fp))
{
	struct inode *ip;
	dev_t dev;
	int major;
	int rw;
	
	if (!fp || --fp->f_count > 0)
		return;
	
	ip = fp->f_inode;
	dev = ip->i_rdev;
	major = MAJOR(dev);
	rw = fp->f_flag;
	
	plock(ip);
	if (fp->f_type == DTYPE_PIPE) {
		ip->i_mode &= ~(IREAD | IWRITE);
		wakeup(ip+1); /* XXX */
		wakeup(ip+2);
	}
	//iput(ip);
	
	switch (ip->i_mode & IFMT) {
	case IFCHR:
		cdevsw[major].d_close(dev, rw);
		break;
	case IFBLK:
		bdevsw[major].d_close(dev, rw);
	}
}

STARTUP(void openf(struct file *fp, int rw))
{
	dev_t dev;
	int major;
	struct inode *ip;
	
	ip = fp->f_inode;
	dev = ip->i_dev;
	major = MAJOR(dev);
	
	switch (ip->i_mode & IFMT) {
	case IFCHR:
		if (major == 255) {
			/* file descriptor pseudo-device in /dev/fd/ */
			int fd = MINOR(dev);
			struct inode *ip2;
			struct file *fp2;
			
			fp2 = getf(fd);
			if (!fp2 || fp == fp2) {
				/* this fd is not open or this file is the one
				 * we just opened */
				goto bad;
			}
			ip = fp2->f_inode;
			
			/*
			 * redirect this file entry so it points to the inode of the
			 * corresponding open file
			 */
			iput(ip); /* put the old inode */
			++ip2->i_count;
			fp->f_inode = ip2;
			/* XXX: do we have to set/check file permissions too? */
			
			break;
		}
		if (major >= nchrdev)
			goto bad;
		cdevsw[major].d_open(dev, rw);
		break;
	case IFBLK:
		if (major >= nchrdev)
			goto bad;
		bdevsw[major].d_open(dev, rw);
	}
	
	return;
	
bad:
	P.p_error = ENXIO;
}

STARTUP(int canaccess(struct inode *ip, int mode))
{
	extern struct fs *getfs(dev_t); /* XXX put this in a header */
	
	if (mode == IWRITE) {
		if (getfs(ip->i_dev)->fs_ronly) {
			P.p_error = EROFS;
			return 1;
		}
		if (ip->i_flag & ITEXT) {
			P.p_error = ETXTBSY;
			return 1;
		}
	}
	if (P.p_euid == 0) {
		if (mode == IEXEC && (ip->i_mode &
		 (IEXEC | (IEXEC >> 3) | (IEXEC >> 6))) == 0)
			goto bad;
		return 0;
	}
	/* check whether we are the owner or in the group of the file */
	if (P.p_euid != ip->i_uid) {
		mode >>= 3;
		if (P.p_egid != ip->i_gid)
			mode >>= 3;
	}
	if ((ip->i_mode & mode) != 0)
		return 0;

bad:
	P.p_error = EACCES;
	return 1;
}
#endif /* TAINTED */

/* allocate a file descriptor */
/* start searching at 'd' */
STARTUP(int fdalloc(int d))
{
	/* look for lowest free fd */
	for (; d < NOFILE; ++d) {
		if (P.p_ofile[d] == NULL) {
			P.p_retval = d;
			P.p_oflag[d] = 0;
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
	kprintf("no file\n");
	P.p_error = ENFILE;
	return -1;
}
