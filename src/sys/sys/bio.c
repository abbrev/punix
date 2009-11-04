/* from v6 */

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
*/
#include <string.h>
#include <assert.h>

#include "punix.h"
#include "process.h"
/*
#include "flash.h"
*/
#include "buf.h"
#include "proc.h"
#include "buf.h"
#include "dev.h"
#include "globals.h"

#define DEBUG()

STARTUP(struct buf *bufalloc())
{
	size_t sb = sizeof(struct buf);
	size_t sd = BLOCKSIZE;
	struct buf *bp;
	char *cp;
	
	bp = memalloc(&sb, 0);
	if (!bp) goto end;
	cp = memalloc(&sd, 0);
	if (!cp) goto freebp;
	
	++G.numbufs;
	bp->b_flags = 0;
	bp->b_dev = 0;
	bp->b_bcount = 0;
	bp->b_addr = cp;
	bp->b_blkno = 0;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_next = bp->b_prev = bp;
	
	/* insert this buf at the beginning of the available buf list */
	bp->b_avnext = G.avbuflist.b_avnext;
	bp->b_avprev = &G.avbuflist;
	G.avbuflist.b_avnext->b_avprev = bp;
	G.avbuflist.b_avnext = bp;
	goto end;

freebp:
	memfree(bp, 0);
end:
	return bp;
}

/* free a buffer if possible, and return 1 if it was freed */
STARTUP(int buffree(struct buf *bp))
{
	if (G.numbufs <= MINBUF) return 0;
	
	/* remove this buf from each list */
	bp->b_next->b_prev = bp->b_prev;
	bp->b_prev->b_next = bp->b_next;
	bp->b_avnext->b_avprev = bp->b_avprev;
	bp->b_avprev->b_avnext = bp->b_avnext;
	memfree(bp->b_addr, 0);
	memfree(bp, 0);
	--G.numbufs;
	return 1;
}

/* FIXME: finish this */
STARTUP(void brelse(struct buf *bp))
{
	int sps;
	
	if (bp->b_flags & B_WANTED)
		wakeup(bp);
	if (G.avbuflist.b_flags & B_WANTED)
		wakeup(&G.avbuflist);
	/* ... */
	if (bp->b_flags & B_ERROR)
		MINOR(bp->b_dev) = -1;
	/* ... */
	if (bp->b_flags & B_BUSY) {
		/* insert this buf at the end of the avbuflist */
		bp->b_avnext = &G.avbuflist;
		bp->b_avprev = G.avbuflist.b_avprev;
		G.avbuflist.b_avprev->b_avnext = bp;
		G.avbuflist.b_avprev = bp;
	}
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

/* take this buf off of the available buf list and set its B_BUSY flag */
STARTUP(static void notavail(struct buf *bp))
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
		slp(bp, 0);
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

STARTUP(void clrbuf(struct buf *bp))
{
	int i;
	int n = 0;
	
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
			slp(bp , 0);
			goto loop;
		}
		
		notavail(bp);
		return bp;
	}
	
	/* we couldn't find an associated buffer, so look in the available
	 * buffer list */
	
	/* the list is empty, so sleep on it */
	if (G.avbuflist.b_avnext == &G.avbuflist && !bufalloc()) {
		G.avbuflist.b_flags |= B_WANTED;
		slp(&G.avbuflist, 0);
		goto loop;
	}
	notavail(bp = G.avbuflist.b_avnext);
	
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

STARTUP(void bufinit())
{
	G.avbuflist.b_avnext = G.avbuflist.b_avprev = &G.avbuflist;
	G.numbufs = 0;
	while (G.numbufs < MINBUF)
		assert(bufalloc());
}
