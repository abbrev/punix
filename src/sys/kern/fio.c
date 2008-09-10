/*
 * Punix, Puny Unix kernel
 * Copyright 2007 Chris Williams
 * 
 * $Id: fio.c,v 1.6 2008/01/15 20:06:00 fredfoobar Exp $
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
#include "filsys.h"
#include "inode.h"
#include "buf.h"
#include "dev.h"
#include "queue.h"
#include "globals.h"


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
	dev = ip->i_dev;
	major = MAJOR(dev);
	rw = fp->f_flag;
	
	plock(ip);
	if (fp->f_type == DTYPE_PIPE) {
		ip->i_mode &= ~(IREAD | IWRITE);
		wakeup(ip+1); /* XXX */
		wakeup(ip+2);
	}
	iput(ip);
	
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
	extern struct filsys *getfs(dev_t); /* XXX put this in a header */
	
	if (mode == IWRITE) {
		if (getfs(ip->i_dev)->s_ronly) {
			P.p_error = EROFS;
			return 1;
		}
		if (ip->i_flag & ITEXT) {
			P.p_error = ETXTBSY;
			return 1;
		}
	}
	if (P.p_uid == 0) {
		if (mode == IEXEC && (ip->i_mode &
		 (IEXEC | (IEXEC >> 3) | (IEXEC >> 6))) == 0)
			goto bad;
		return 0;
	}
	/* check whether we are the owner or in the group of the file */
	if (P.p_uid != ip->i_uid) {
		mode >>= 3;
		if (P.p_gid != ip->i_gid)
			mode >>= 3;
	}
	if ((ip->i_mode & mode) != 0)
		return 0;

bad:
	P.p_error = EACCES;
	return 1;
}

/* allocate a file descriptor */
/* start searching at 'd' */
STARTUP(int ufalloc(int d))
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
	
	if ((fd = ufalloc(0)) < 0)
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
