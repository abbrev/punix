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

void i_trunc(struct inode *inop)
{
	pfs_inode_trunc(inop->i_dev, inop->i_number);
}

struct inode *i_alloc(dev_t dev)
{
	struct inode *inop;
	ino_t ino;
	ino = pfs_inode_alloc(dev); /* XXX: vfs supports only PFS */
	if (!ino) return NULL;
	
	/* TODO: write this */
	return NULL;
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
	/* FIXME */
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

// get an inode from disk, return an inode
struct inode *i_open(dev_t dev, ino_t ino)
{
	struct inode *inop;
	
	if (ino == NULLINO) {
		inop = i_alloc(dev);
		/* FIXME */
	}
	return NULL;
}

/* expand a disk inode structure to core inode */
#if 0 /* REWRITE */
STARTUP(void iexpand(struct inode *ip, struct dinode *dp))
{
	/* FIXME */
}
#endif


/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * printf warning: no inodes -- if the inode
 *	structure is full
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
#if 0 /* REWRITE */
STARTUP(struct inode *iget(dev_t dev, struct fs *fs, ino_t ino))
{
	/* body deleted */
	return NULL;
}
#endif /* REWRITE */

/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */
#if 0 /* REWRITE */
STARTUP(void iput(struct inode *ip))
{
	/* body deleted */
}
#endif /* REWRITE */

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

#if 0
/* note! this duplicates plock */
#endif
