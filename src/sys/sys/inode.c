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

/*
 * TODO: put inodes on lists when they are allocated. Each filesystem object
 * will have a list of inodes that belong to it. All inodes in these lists have
 * non-zero reference counts. There will also be a global list of inodes that
 * are currently unused (in LRU order) but kept around for some time as a
 * cache (so they don't have to be read from device again). iget() will look
 * through the filesystem's list first, and then look through the unused inode
 * list second. If a usable inode is not found, a new inode will be allocated
 * from the heap and added to the filesystem's inode list.
 *
 * The size of the unused inode list must be maintained in order to keep it
 * under control. The size should be kept either as a percentage of the total
 * inode count, or as an absolute maximum size, or both (limit the size to the
 * minimum of both).
 */

/*
 * create a new vfs inode object with fs and num initialized
 * return with inode locked
 */
static struct inode *new_inode(struct filesystem *fs, ino_t num)
{
	struct inode *ip;
	size_t size = sizeof(struct inode);
	ip = memalloc(&size, 0);
	if (!ip) {
		P.p_error = ENFILE;
		return NULL;
	}
	ip->i_fs = fs;
	ip->i_num = num;
	ip->i_count = 0;
	ip->i_flag = ILOCKED;
	return ip;
}

/* free a vfs inode object */
static void delete_inode(struct inode *ip)
{
	assert(ip->i_count == 0); // make sure nobody is referencing this inode
	memfree(ip, 0);
}

void inodeinit()
{
	struct inode *ip;
	for (ip = &G.inode[0]; ip < &G.inode[NINODE]; ++ip) {
		ip->i_fs = NULL;
		ip->i_flag = 0;
		ip->i_count = 0;
	}
}

/*
 * Get an inode by filesystem/inum pair.
 * If inode is already in memory, lock it and return it.
 * Otherwise, read it in from the device, lock it, and return it.
 * If the inode is mounted on, return the inode that covers it.
 */
/* TODO: follow mount points */
struct inode *iget(struct filesystem *fs, ino_t num)
{
	struct inode *ip;
	struct inode *freeip;

retry:
	freeip = NULL;
	for (ip = &G.inode[0]; ip < &G.inode[NINODE]; ++ip) {
		if (ip->i_fs == fs && ip->i_num == num) {
			if (ip->i_flag & ILOCKED) {
				ip->i_flag |= IWANTED;
				slp(ip, 0);
				goto retry;
			}
			goto out;
		}
		if (ip->i_count == 0)
			freeip = ip;
	}
	if (!(ip = freeip)) {
		kprintf("no free inodes!\n");
		P.p_error = ENFILE;
		return NULL;
	}
	ip->i_fs = fs;
	ip->i_num = num;

out:
	ip->i_flag |= ILOCKED;
	++ip->i_count;
	return ip;
}

/*
 * Decrement the reference count.
 * If reference count becomes 0, write it out to
 * the device or delete the file if nlink is also 0.
 */
void iput(struct inode *ip)
{
	if (--ip->i_count) return;

	// TODO: write this
}

STARTUP(void ilock(struct inode *inop))
{
	while (inop->i_flag & ILOCKED) {
		inop->i_flag |= IWANTED;
		slp(inop, 0);
	}
	inop->i_flag |= ILOCKED;
}

STARTUP(void iunlock(struct inode *inop))
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
		// TODO: delete the inode on disk
	}
}

