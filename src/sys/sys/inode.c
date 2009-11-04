/* This comes from V7/usr/sys/sys/iget.c */
/* FIXME: clean it up */

#include <time.h>
#include <stddef.h>
#include <errno.h>

#include "punix.h"
#include "param.h"
/*
#include "systm.h"
#include "user.h"
*/
#include "inode.h"
/*
#include "mount.h"
#include "fs.h"
#include "conf.h"
*/
#include "buf.h"
#include "queue.h"
#include "proc.h"
#include "globals.h"

void iput(struct inode *p);
/*void iupdat(int *p, int *tm);*/
void itrunc(struct inode *ip);
struct inode *maknode(int mode);
void wdir(struct inode *ip);

struct dinode;

/* create a new inode structure for the given device and inum
 * return with inode locked */
struct inode *i_new(dev_t dev, ino_t ino)
{
	struct inode *ip;
	size_t isize = sizeof(struct inode);
	ip = memalloc(&isize, 0);
	if (!ip) {
		P.p_error = ENFILE;
		return NULL;
	}
	ip->i_dev = dev;
	ip->i_number = ino;
	ip->i_fs = getfs(dev);
	ip->i_flag = 0;
	ip->i_count = 0;
	i_ref(ip);
	I_LOCK(ip);
	list_add(&ip->i_list, &G.inode_list);
	return ip;
}

/* delete an inode; assumes that it has no references */
void i_delete(struct inode *ip)
{
	list_del(&ip->i_list);
}

void i_trunc(struct inode *inop)
{
	pfs_inode_trunc(inop->i_dev, inop->i_number);
}

/* allocate an inode on device and return with a locked inode */
struct inode *i_alloc(dev_t dev)
{
	struct inode *inop;
	ino_t ino;
	ino = pfs_inode_alloc(dev);
	if (!ino) return NULL;
	
	/* TODO: write this */
	return NULL;
	
#if 0
	struct inode *ip;
	ino_t ino;
	ino = pfs_inode_alloc(dev); /* XXX: vfs supports only PFS */
	if (!ino) return NULL;
	ip = i_new(dev, ino);
	if (!ip) {
		i_free(dev, ino);
		return NULL;
	}
	return ip;
#endif
}

void i_free(dev_t dev, ino_t ino)
{
	pfs_inode_free(dev, ino); /* XXX: vfs supports only PFS */
}

STARTUP(void i_lock(struct inode *inop))
{
	while (inop->i_flag & ILOCKED) {
		inop->i_flag |= IWANTED;
		slp(inop, PINOD);
	}
	inop->i_flag |= ILOCKED;
}

STARTUP(void i_unlock(struct inode *inop))
{
	inop->i_flag &= ~ILOCKED;
	if (inop->i_flag & IWANTED) {
		inop->i_flag &= ~IWANTED;
		wakeup(inop);
	}
}
// increase reference count
void i_ref(struct inode *inop)
{
	++inop->i_count;
}

// decrease reference count, freeing it on disk if nlink=0
void i_unref(struct inode *inop)
{
	if (--inop->i_count) return;
	/* FIXME */
	if (inop->i_nlink == 0) {
		i_trunc(inop);
		i_free(inop->i_dev, inop->i_number);
	}
}

/* read the inode from device */
int i_read(struct inode *ip)
{
	return pfs_readinode(ip);
}

/* get an inode, possibly reading it from disk, and return it locked */
struct inode *i_open(dev_t dev, ino_t ino)
{
	struct inode *ip;
	size_t isize = sizeof(struct inode);
	
	if (ino == NULLINO)
		return i_alloc(dev);
	
	/* look for the specified inode in memory */
	list_for_each_entry(ip, &G.inode_list, i_list) {
		if (ip->i_dev == dev && ip->i_number == ino) {
			/* we found it */
			i_ref(ip);
			I_LOCK(ip);
			return ip;
		}
	}
	
	/* it's not in memory already.
	 * make a new one and read it from the device */
	ip = i_new(dev, ino);
	if (!ip) return NULL;
	if (i_read(ip)) {
		i_delete(ip);
		return NULL;
	}
	return ip;
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If either is on, update the inode
 * with the corresponding dates
 * set to the argument tm.
 */
#if 0 /* REWRITE */
STARTUP(void iupdat(struct inode *ip, time_t *ta, time_t *tm))
{
	/* body deleted */
}
#endif /* REWRITE */

/*
 * Free all the disk blocks associated
 * with the specified inode structure.
 * The blocks of the file are removed
 * in reverse order. This FILO
 * algorithm will tend to maintain
 * a contiguous free list much longer
 * than FIFO.
 */
#if 0 /* REWRITE */
void itrunc(struct inode *ip)
{
	/* body deleted */
}
#endif /* REWRITE */

/*
 * Make a new file.
 */
#if 0 /* REWRITE */
STARTUP(struct inode *maknode(int mode))
{
	/* body deleted */
	return NULL;
}
#endif /* REWRITE */

/*
 * Write a directory entry with
 * parameters left as side effects
 * to a call to namei.
 */
#if 0 /* REWRITE */
STARTUP(void wdir(struct inode *ip))
{
	/* body deleted */
}
#endif /* REWRITE */
