/* $Id: sys_inode.c,v 1.6 2008/01/04 03:26:05 fredfoobar Exp $ */

#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>

#include "punix.h"
#include "param.h"
#include "proc.h"
#include "inode.h"
#include "file.h"
#include "buf.h"
#include "dev.h"
#include "queue.h"
#include "globals.h"
/*
#include "uio.h"
#include "stat.h"
#include "mount.h"
#include "ioctl.h"
*/

STARTUP(void openi(struct inode *ip, int rw))
{
	dev_t dev;
	int maj;
	
	dev = ip->i_rdev;
	maj = MAJOR(dev);
	
	switch (ip->i_mode & IFMT) {
	case IFCHR:
		if (maj >= nchrdev)
			goto bad;
		cdevsw[maj].d_open(dev, rw);
		break;
	case IFBLK:
		if (maj >= nblkdev)
			goto bad;
		bdevsw[maj].d_open(dev, rw);
		break;
	}
	return;
bad:
	P.p_error = ENXIO;
}

STARTUP(void closei(struct inode *ip, int rw))
{
	dev_t dev;
	int maj;
	
	dev = ip->i_rdev;
	maj = MAJOR(dev);
	
	if (ip->i_count <= 1) {
		switch (ip->i_mode & IFMT) {
		case IFCHR:
			cdevsw[maj].d_close(dev, rw);
			break;
		case IFBLK:
			bdevsw[maj].d_close(dev, rw);
		}
	}
	
	iput(ip);
}

#if 0
/* This is from Uzix */
/* Getinode() returns the inode pointer associated with a user's
 * file descriptor, checking for valid data structures
 */
inoptr getinode(int fd)
{
	inoptr inoindex;
	struct file *fp; //uchar oftindex = UDATA(u_files)[fd];
	
	GETF(fp, fd);
	/*if (oftindex >= OFTSIZE) // check that fp is within bounds
		panic(gtinobadoft, oftindex, " for", fd);*/
	if ((inoindex = of_tab[oftindex].o_inode) < i_tab ||
	    inoindex >= i_tab + ITABSIZE)
		panic(gtinobadoft, oftindex, ", inode", inoindex);
	magic(inoindex);
	return inoindex;
}
#endif
