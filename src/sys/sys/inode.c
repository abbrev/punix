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

/* expand a disk inode structure to core inode */
STARTUP(void iexpand(struct inode *ip, struct dinode *dp))
{
	/* FIXME */
}

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
STARTUP(struct inode *iget(dev_t dev, struct fs *fs, ino_t ino))
{
	struct inode *ip;
	struct inode *oip;
	struct mount *mp;
	struct buf *bp;
	struct dinode *dp;

loop:
	oip = NULL;
	for EACHINODE(ip) {
		if (dev == ip->i_dev && ino == ip->i_number) {
			if (ip->i_flag & ILOCKED) {
				ip->i_flag |= IWANT;
				slp(ip, PINOD);
				goto loop;
			}
#if 0
			if (ip->i_flag & IMOUNT) {
				for EACHMOUNT(mp)
					if (mp->m_inodp == ip) {
						dev = mp->m_dev;
						ino = ROOTINO;
						goto loop;
					}
				panic("no imt");
			}
#endif
			++ip->i_count;
			ip->i_flag |= ILOCKED;
			return ip;
		}
		if (oip == NULL && ip->i_count == 0)
			oip = ip;
	}
	ip = oip;
	if (ip == NULL) {
		kprintf("Inode table overflow\n");
		P.p_error = ENFILE;
		return NULL;
	}
	ip->i_dev = dev;
	ip->i_fs = fs;
	ip->i_number = ino;
	ip->i_flag = ILOCKED;
	++ip->i_count;
	/*ip->i_lastr = 0;*/
	bp = bread(dev, itod(ino)); /* XXX */
	/*
	 * Check I/O errors
	 */
#if 0
	if (mp->b_flags & B_ERROR) {
		brelse(bp);
		iput(ip);
		return NULL;
	}
#endif
	dp = (struct inode *)(bp->b_addr + itoo(ino));
	iexpand(ip, dp);
	brelse(bp);
	return ip;
}

/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */
STARTUP(void iput(struct inode *ip))
{
	if (ip->i_count == 1) {
		ip->i_flag |= ILOCKED;
		if (ip->i_nlink <= 0) {
			itrunc(ip);
			ip->i_mode = 0;
			ip->i_flag |= IUPD | ICHG;
			ifree(ip->i_dev, ip->i_number);
		}
		iupdat(ip, &walltime.tv_sec, &walltime.tv_sec);
		prele(ip);
		ip->i_flag = 0;
		ip->i_number = 0;
	}
	--ip->i_count;
	prele(ip);
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If either is on, update the inode
 * with the corresponding dates
 * set to the argument tm.
 */
STARTUP(void iupdat(struct inode *ip, time_t *ta, time_t *tm))
{
#if 0
	struct buf *bp;
	struct dinode *dp;
	char *p1, *p2;
	int i;
	
	if (ip->i_flag & (IUPD | IACC | ICHG)) {
		if (getfs(ip->i_dev)->fs_ronly)
			return;
		
		bp = bread(ip->i_dev, itod(ip->i_number));
		
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		dp = bp->b_dino + itoo(ip->i_number);
		dp->di_mode = ip->mode;
		dp->di_nlink = ip->i_nlink;
		dp->di_uid = ip->i_uid;
		dp->di_gid = ip->i_gid;
		dp->di_size = ip->i_size;
		
		/* ... */
		
		if (ip->i_flag & IACC)
			dp->di_atime = *ta;
		if (ip->i_flag & IPUD)
			dp->di_mtime = *tm;
		if (ip->i_flag & ICHG)
			dp->di_ctime = time;
		ip->i_flag &= ~(IUPD | IACC | ICHG);
		bdwrite(bp);
	}
#endif
}

/*
 * Free all the disk blocks associated
 * with the specified inode structure.
 * The blocks of the file are removed
 * in reverse order. This FILO
 * algorithm will tend to maintain
 * a contiguous free list much longer
 * than FIFO.
 */
void itrunc(struct inode *ip)
{
#if 0
	int *bp;
	int *cp;
	int *dp;
	int *ep;
	struct inode *rp;
	
	if ((ip->i_mode&(IFCHR&IFBLK)) != 0)
		return;
	for (rp = &ip->i_addr[7]; rp >= &ip->i_addr[0]; --rp)
	if (*rp) {
		if ((ip->i_mode&ILARG) != 0) {
			bp = bread(ip->i_dev, *rp);
			for (cp = bp->b_addr+512; cp >= bp->b_addr; --cp)
			if (*cp) {
				if (rp == &ip->i_addr[7]) {
					dp = bread(ip->i_dev, *cp);
					for (ep = dp->b_addr+512; ep >= dp->b_addr; --ep)
					if (*ep)
						free(ip->i_dev, *ep);
					brelse(dp);
				}
				free(ip->i_dev, *cp);
			}
			brelse(bp);
		}
		free(ip->i_dev, *rp);
		*rp = 0;
	}
	ip->i_mode &= ~ILARG;
	ip->i_size = 0;
	ip->i_flag |= IUPD;
#endif
}

/*
 * Make a new file.
 */
STARTUP(struct inode *maknode(int mode))
{
	struct inode *ip;
	
	ip = ialloc(P.p_pdir->i_dev);
	if (ip == NULL)
		return NULL;
	
	ip->i_flag |= IACC|IUPD|ICHG;
	
	if ((mode & IFMT) == 0)
		mode |= IFREG;
	
	ip->i_mode = mode & ~P.p_cmask;
	ip->i_nlink = 1;
	ip->i_uid = P.p_uid;
	ip->i_gid = P.p_gid;
	wdir(ip);
	return ip;
}

/*
 * Write a directory entry with
 * parameters left as side effects
 * to a call to namei.
 */
STARTUP(void wdir(struct inode *ip))
{
	register char *cp1, *cp2;
	
	P.p_dent.d_ino = ip->i_number;
	cp1 = &P.p_dent.d_name[0];
	for(cp2 = &P.p_dbuf[0]; cp2 < &P.p_dbuf[NAME_MAX];)
		*cp1++ = *cp2++;
	P.p_count = NAME_MAX+2;
	P.p_base = &P.p_dent;
	writei(P.p_pdir);
	iput(P.p_pdir);
}

/* note! this duplicates plock */
STARTUP(void ilock(struct inode *ip))
{
	while (ip->i_flag & ILOCKED) {
		ip->i_flag |= IWANT;
		slp(ip, PINOD);
	}
	ip->i_flag |= ILOCKED;
}

STARTUP(void iunlock(struct inode *ip))
{
	ip->i_flag &= ~ILOCKED;
	if (ip->i_flag & IWANT) {
		ip->i_flag &= ~IWANT;
		wakeup(ip);
	}
}
