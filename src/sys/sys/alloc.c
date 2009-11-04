#include <string.h>

#include "punix.h"
#include "proc.h"
#include "buf.h"
#include "dev.h"
#include "fs.h"
#include "inode.h"
#include "mount.h"
#include "queue.h"
#include "globals.h"

/* FIXME: from v7. fix it up yo */

#define prdev(s, dev) ((void)0)

typedef	struct fblk *FBLKP;

/* FIXME: rewrite ialloc for PFS */
/*
 * Allocate an unused I node on the specified device.
 * Used with file creation. The algorithm keeps up to
 * NICINOD spare I nodes in the super block. When this runs out,
 * a linear search through the I list is instituted to pick
 * up NICINOD more.
 */
STARTUP(struct inode *ialloc(dev_t dev))
{
	return NULL;
#if 0
	struct fs *fp;
	struct buf *bp;
	struct inode *ip;
	int i;
	struct dinode *dp;
	ino_t ino;
	daddr_t adr;
	
	fp = getfs(dev);
	while (fp->fs_ilock)
		slp(&fp->fs_ilock, 0);
loop:
	if (fp->fs_ninode > 0) {
		ino = fp->fs_inode[--fp->fs_ninode];
		if (ino < ROOTINO)
			goto loop;
		ip = iget(dev, ino);
		if (ip == NULL)
			return NULL;
		if (ip->i_mode == 0) {
			for (i=0; i<NADDR; ++i)
				ip->i_un.i_addr[i] = 0;
			fp->fs_fmod = 1;
			return(ip);
		}
		/*
		 * Inode was allocated after all.
		 * Look some more.
		 */
		iput(ip);
		goto loop;
	}
	fp->fs_ilock++;
	ino = 1;
	for(adr = SUPERB+1; adr < fp->fs_isize; adr++) {
		bp = bread(dev, adr);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			ino += INOPB;
			continue;
		}
		dp = bp->b_un.b_dino;
		for(i=0; i<INOPB; i++) {
			if(dp->di_mode != 0)
				goto cont;
			for EACHINODE(ip)
				if (dev==ip->i_dev && ino==ip->i_number)
					goto cont;
			fp->fs_inode[fp->fs_ninode++] = ino;
			if(fp->fs_ninode >= NICINOD)
				break;
		cont:
			ino++;
			dp++;
		}
		brelse(bp);
		if(fp->fs_ninode >= NICINOD)
			break;
	}
	fp->fs_ilock = 0;
	wakeup((caddr_t)&fp->fs_ilock);
	if(fp->fs_ninode > 0)
		goto loop;
	prdev("Out of inodes", dev);
	u.u_error = ENOSPC;
	return(NULL);
#endif
}

/* FIXME: rewrite ifree for PFS */
/*
 * Free the specified I node on the specified device.
 * The algorithm stores up to NICINOD I nodes in the super
 * block and throws away any more.
 */
STARTUP(void ifree(dev_t dev, ino_t ino))
{
#if 0
	register struct fs *fp;

	fp = getfs(dev);
	if (fp->fs_ilock)
		return;
	if (fp->fs_ninode >= NICINOD)
		return;
	fp->fs_inode[fp->fs_ninode++] = ino;
	fp->fs_fmod = 1;
#endif
}

/*
 * getfs maps a device number into
 * a pointer to the incore super
 * block.
 * The algorithm is a linear
 * search through the mount table.
 * A consistency check of the
 * in core free-block and i-node
 * counts.
 *
 * bad count on dev x/y -- the count
 *	check failed. At this point, all
 *	the counts are zeroed which will
 *	almost certainly lead to "no space"
 *	diagnostic
 * panic: no fs -- the device is not mounted.
 *	this "cannot happen"
 */
STARTUP(struct fs *getfs(dev_t dev))
{
	struct mount *mp;
	
	for EACHMOUNT(mp) {
		if (mp->m_filsys != NULL && mp->m_dev == dev) {
			struct fs *fp = mp->m_filsys;
			if (fp->fs_nfree > NICFREE || fp->fs_ninode > NICINOD) {
				prdev("bad count", dev);
				fp->fs_nfree = 0;
				fp->fs_ninode = 0;
			}
			return fp;
		}
	}
	panic("no fs");
	return NULL;
}

/*
 * update is the internal name of
 * 'sync'. It goes through the disk
 * queues to initiate sandbagged IO;
 * goes through the I nodes to write
 * modified nodes; and it goes through
 * the mount table to initiate modified
 * super blocks.
 */
STARTUP(void update())
{
	register struct inode *ip;
	register struct mount *mp;
	register struct buf *bp;
	struct fs *fp;
	
	if (updlock)
		return;
	
	++updlock;
	
	for EACHMOUNT(mp)
		if (mp->m_filsys != NULL) {
			fp = mp->m_filsys;
			if (fp->fs_fmod == 0 || fp->fs_ilock != 0 ||
			    fp->fs_flock != 0 || fp->fs_ronly != 0)
				continue;
			
			bp = getblk(mp->m_dev, SUPERB);
			
			if (bp->b_flags & B_ERROR)
				continue;
			
			fp->fs_fmod = 0;
			fp->fs_time = realtime.tv_sec;
			memcpy(bp->b_addr, fp, BSIZE);
			bwrite(bp);
		}
	
	for EACHINODE(ip)
		if ((ip->i_flag & ILOCKED) == 0 && ip->i_count != 0) {
			ip->i_flag |= ILOCKED;
			++ip->i_count;
			iupdat(ip, &realtime.tv_sec, &realtime.tv_sec);
			iput(ip);
		}
	
	updlock = 0;
	bflush(NODEV);
}
