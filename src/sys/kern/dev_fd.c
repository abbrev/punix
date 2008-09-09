/*
 * dev_fd.c, file descriptor devices
 * Copyright 2008 Christopher Williams
 * 
 * $Id$
 * 
 * This implements device files in /dev/fd/.
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

STARTUP(void devfdopen(struct file *fp, int rw))
{
	dev_t dev = fp->f_inode->i_dev;
	int fd = MINOR(dev);
	struct inode *ip;
	struct file *fp2;
	
	fp2 = getf(fd);
	if (!fp2 || fp == fp2) {
		/* this fd is not open or this file is the one we just opened */
		P.p_error = ENXIO;
		return;
	}
	ip = fp2->f_inode;
	
	/*
	 * redirect this file entry so it points to the inode of the
	 * corresponding open file
	 */
	iput(fp->f_inode); /* put the old inode */
	++ip->i_count;
	fp->f_inode = ip;
}
