#include <errno.h>

#include "punix.h"
#include "proc.h"
#include "inode.h"
#include "buf.h"
#include "dev.h"
#include "queue.h"
#include "globals.h"

/* this is copied almost directly from V7 */

/*
 * Return the logical maximum
 * of the 2 arguments.
 */
STARTUP(unsigned max(unsigned a, unsigned b))
{
	if (a > b)
		return a;
	return b;
}

/*
 * Return the logical minimum
 * of the 2 arguments.
 */
STARTUP(unsigned min(unsigned a, unsigned b))
{
	if (a < b)
		return a;
	return b;
}

STARTUP(void iomove(void *cp, int n, int flag))
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

/*
 * Read the file corresponding to
 * the inode pointed at by the argument.
 * The actual read arguments are found
 * in the variables:
 *      p_base          core address for destination
 *      p_offset        byte offset in file
 *      p_count         number of bytes to read
 */
STARTUP(void readi(struct inode *ip))
{
	struct buf *bp;
	dev_t dev;
	long lbn, bn;
	off_t diff;
	int on, n;
	int type;
	
	if (P.p_count == 0)
		return;
	if (P.p_offset < 0) {
		P.p_error = EINVAL;
		return;
	}
	ip->i_flag |= IACC;
	
	dev = ip->i_rdev;
	type = ip->i_mode & IFMT;
	
	if (type == IFCHR) {
		cdevsw[MAJOR(dev)].d_read(dev);
		return;
	}
	
	do {
		lbn = bn = P.p_offset >> BLOCKSHIFT;
		on = P.p_offset & BLOCKMASK;
		n = min((unsigned)(BLOCKSIZE-on), P.p_count);
		if (type != IFBLK) {
			diff = ip->i_size - P.p_offset;
			if (diff <= 0)
				return;
			if (diff < n)
				n = diff;
			bn = bmap(ip, bn, B_READ);
			if (P.p_error)
				return;
			dev = ip->i_dev;
		}
		if ((long)bn < 0) {
			bp = getblk(NODEV, 0);
			clrbuf(bp);
		} else
			bp = bread(dev, bn);
		n = min((unsigned)n, BLOCKSIZE-bp->b_resid);
		if (n != 0)
			iomove(bp->b_addr + on, n, B_READ);
		brelse(bp);
	} while (!P.p_error && P.p_count != 0 && n > 0);
}

STARTUP(void writei(struct inode *ip))
{
	struct buf *bp;
	dev_t dev;
	long bn;
	int n, on;
	int type;
	
	if (P.p_offset < 0) {
		P.p_error = EINVAL;
		return;
	}
	dev = ip->i_rdev;
	type = ip->i_mode & IFMT;
	if (type == IFCHR) {
		ip->i_flag |= IUPD|ICHG;
		cdevsw[MAJOR(dev)].d_write(dev);
		return;
	}
	if (P.p_count == 0)
		return;
	
	do {
		bn = P.p_offset >> BLOCKSHIFT;
		on = P.p_offset & BLOCKMASK;
		n = min((unsigned)(BLOCKSIZE-on), P.p_count);
		if (type != IFBLK) {
			bn = bmap(ip, bn, B_WRITE);
			if ((long)bn < 0)
				return;
			dev = ip->i_dev;
		}
		
		if (n == BLOCKSIZE)	/* we write over the whole block */
			bp = getblk(dev, bn);
		else
			bp = bread(dev, bn);
		iomove(bp->b_addr + on, n, B_WRITE);
		if (P.p_error)
			brelse(bp);
		else
			bdwrite(bp);
		if (P.p_offset > ip->i_size && (type == IFDIR || type == IFREG))
			ip->i_size = P.p_offset;
		ip->i_flag |= IUPD|ICHG;
	} while (!P.p_error && P.p_count != 0);
}
