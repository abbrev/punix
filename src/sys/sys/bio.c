/* from v6 */

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
*/
#include <string.h>

#include "punix.h"
#include "process.h"
/*
#include "flash.h"
*/
#include "buf.h"
#include "proc.h"
#include "buf.h"
#include "dev.h"

#define DEBUG()

struct buf avbuflist; /* list of buf */
static struct buffer buffers[NBUF];

STARTUP(char *bufalloc())
{
	struct buffer *bfp;
	for (bfp = &buffers[0]; bfp < &buffers[NBUF]; ++bfp) {
		if (!bfp->used) {
			bfp->used = 1;
			return bfp->data;
		}
	}
	return NULL;
}

STARTUP(void buffree(char *cp))
{
	struct buffer *bfp = (struct buffer *)cp;
	bfp->used = 0;
}

/* FIXME: finish this */
STARTUP(void brelse(struct buf *bp))
{
	int sps;
	
	if (bp->b_flags & B_WANTED)
		/*wakeup(rbp)*/;
	/* ... */
	if (bp->b_flags & B_ERROR)
		MINOR(bp->b_dev) = -1;
	/* ... */
	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC);
	/* ... */
	bp->b_next->b_prev = bp->b_prev;
	bp->b_prev->b_next = bp->b_next;
}

STARTUP(struct buf *incore(dev_t dev, long blkno))
{
	struct buf *bp;
	const struct devtab *dp;
	
	dp = bdevsw[MAJOR(dev)].d_tab;
	
	for (bp = dp->b_next; bp != (struct buf *)dp; bp = bp->b_next)
		if (bp->b_blkno == blkno && bp->b_dev == dev)
			return bp;
	
	return NULL;
}

STARTUP(void notavail(struct buf *bp))
{
	bp->b_avprev->b_avnext = bp->b_avnext;
	bp->b_avnext->b_avprev = bp->b_avprev;
	bp->b_flags |= B_BUSY;
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
STARTUP(void iowait(struct buf *bp))
{
	/*
	int x = spl6();
	while ((bp->b_flags&B_DONE) == 0)
		slp(bp, PRIBIO);
	splx(x);
	geterror(bp);
	*/
}

STARTUP(struct buf *bread(dev_t dev, long blkno))
{
	struct buf *bp;
	
	bp = getblk(dev, blkno);
	if (bp->b_flags & B_DONE)
		return bp;
	
	bp->b_flags |= B_READ;
	bp->b_bcount = -BLOCKSIZE;
	bdevsw[MAJOR(dev)].d_strategy(bp);
	iowait(bp);
	return bp;
}

STARTUP(void bwrite(struct buf *bp))
{
	int flag;
	
	flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	bp->b_bcount = BLOCKSIZE;
	bdevsw[MAJOR(bp->b_dev)].d_strategy(bp);
	iowait(bp);
	if (bp->b_flags & B_COPY) {
		buffree(bp->b_addr);
	}
	brelse(bp);
	/* */
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 */
STARTUP(void bdwrite(struct buf *bp))
{
	bp->b_flags |= B_DELWRI | B_DONE;
	brelse(bp);
}

/* FIXME: make this do what it should do, not what it currently does do */
STARTUP(void clrbuf(struct buf *bp))
{
	int i;
	int n = 0;
	
	if ((bp->b_flags & B_WRITABLE) == 0) {
		bp->b_addr = bufalloc();
		bp->b_flags |= B_COPY | B_WRITABLE;
	}
	
	if (bp->b_flags & B_CLEAR)
		n = -1;
	
	memset(bp->b_addr, n, BLOCKSIZE);
	
	bp->b_resid = 0;
}

STARTUP(struct buf *getblk(dev_t dev, long blkno))
{
	struct buf *bp;
	struct devtab *dp;
	
	if (MAJOR(dev) >= nblkdev)
		return NULL;
	
	if ((bp = incore(dev, blkno)) != NULL)
		return bp;
	
	/* search the list for the device first, then the av list */
loop:
	dp = bdevsw[MAJOR(dev)].d_tab;
	if (dp == NULL)
		panic("devtab");
	
	for (bp = dp->b_next; bp != (struct buf *)dp; bp = bp->b_next) {
		if (bp->b_blkno != blkno || bp->b_dev != dev)
			continue;
		
		/* this buf is associated with the device/block,
		 * but it's busy */
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			slp(bp , PRIBIO+1);
			goto loop;
		}
		
		notavail(bp);
		return bp;
	}
	
	/* we couldn't find an associated buffer, so look in the available
	 * buffer list */
	
	/* the list is empty, so sleep on it */
	if (avbuflist.b_avnext == &avbuflist) {
		avbuflist.b_flags |= B_WANTED;
		slp(&avbuflist, PRIBIO+1);
		goto loop;
	}
	notavail(bp = avbuflist.b_avnext);
	
	if (bp->b_flags & B_DELWRI) {
		bwrite(bp);
		goto loop;
	}
	bp->b_flags = B_BUSY;
	
	bp->b_prev->b_next = bp->b_next;
	bp->b_next->b_prev = bp->b_prev;
	bp->b_next = dp->b_next;
	bp->b_prev = (struct buf *)dp;
	
	dp->b_next->b_prev = bp;
	dp->b_next = bp;
	
	bp->b_dev = dev;
	bp->b_blkno = blkno;
	
	return bp;
}

STARTUP(void bflush(dev_t dev))
{
}
