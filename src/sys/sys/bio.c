/* clean version of bio.c - block i/o */

/*
 * functions to include in here:
 * V6/V7 name					new Punix name
 *
 * struct buf *bufalloc()			(should be static)
 * int buffree(struct buf *bp)			(should be static)
 * void brelse(struct buf *bp)			blk_release
 * struct buf *incore(dev_t dev, long blkno)	(should be static)
 * static void notavail(struct buf *bp)		(static)
 * void iowait(struct buf *bp)			(should be static)
 * struct buf *bread(dev_t dev, long blkno)	blk_read
 * void bwrite(struct buf *bp)			blk_write
 * void bdwrite(struct buf *bp)			blk_write_delayed
 * void clrbuf(struct buf *bp)			blk_clear
 * struct buf *getblk(dev_t dev, long blkno)	blk_get
 * void bflush(dev_t dev)			blk_flush
 * void bufinit()				bufinit
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

/* allocate a struct buf and initialize it */
static struct buf *bufalloc(void)
{
	size_t sb = sizeof(struct buf);
	size_t sd = BLOCKSIZE;
	struct buf *bufp;
	char *cp;

	bufp = memalloc(&sb, 0);
	if (!bufp) goto end;
	cp = memalloc(&sd, 0);
	if (!cp) goto freebuf;

	++G.numbufs;
	bufp->b_flags = 0;
	bufp->b_dev = 0;
	bufp->b_bcount = 0;
	bufp->b_addr = cp;
	bufp->b_blkno = 0;
	bufp->b_error = 0;
	bufp->b_resid = 0;
	INIT_LIST_HEAD(&bufp->b_list);
	INIT_LIST_HEAD(&bufp->b_avlist);

	/* insert this buf at the beginning of the available buf list */
	list_add(&bufp->b_avlist, &G.avbuf_list);
	goto end;

freebuf:
	memfree(bufp, 0);
	bufp = NULL;
end:
	return bufp;
}

/* attempt to free a buffer. return 1 if one is freed.
 * the buffer system and heap management should ideally be merged to allow for
 * better memory management */
int buffree(struct buf *bufp)
{
	if (G.numbufs <= MINBUF) return 0;
	if (bufp->b_flags & B_DELWRI) blk_write(bufp);
	
	/* remove this buf from each list */
	list_del(&bufp->b_list);
	list_del(&bufp->b_avlist);
	memfree(bufp->b_addr, 0);
	memfree(bufp, 0);
	--G.numbufs;
	return 1;
}

/* if there is a buf on the avbuflist, return it
 * otherwise allocate a new buffer from the heap and return that */
static struct buf *getavbuf(void)
{
	struct buf *bufp;
	
	if (list_empty(&G.avbuf_list)) {
		/* try to allocate a new buffer and put it on the avbuflist */
		bufp = bufalloc();
	} else {
		bufp = list_entry(G.avbuf_list.next, struct buf, b_avlist);
	}
	
	return bufp;
}

static void notavail(struct buf *bufp)
{
	bufp->b_flags |= B_BUSY;
	list_del(&bufp->b_avlist);
}

//#define EACHDEVBUF(devp, devbufp) (devbufp = devp->b_next; devbufp != (struct buf *)devp; devbufp = devbufp->b_next)

/* get a buf associated with a block. always returns a buf. */
/* if there is a buf associated with this block:
 * 	if it's unlocked, lock it and return it
 * 	else wait on it and try again
 * else:
 * 	if there is an available buffer:
 * 		if it's delayed write, write it and try again
 * 		else return it
 *	else sleep on the available list and try again
 */
struct buf *blk_get(dev_t dev, long blkno)
{
	/* TODO: write this */
	struct buf *bufp = NULL;
	struct buf *devbufp;
	struct devtab *devp = bdevsw[MAJOR(dev)].d_tab;
	
tryagain:
	list_for_each_entry(devbufp, &devp->d_buflist, b_list) {
		if (devbufp->b_dev == dev && devbufp->b_blkno == blkno) {
			bufp = devbufp;
			break;
		}
	}
	if (bufp) {
		if (bufp->b_flags & B_BUSY) {
			bufp->b_flags |= B_WANTED;
			slp(bufp, 0);
			goto tryagain;
		} else {
			notavail(bufp);
			goto found;
		}
	}
	bufp = getavbuf();
	if (!bufp) {
		slp(&G.avbuflist, 0);
		goto tryagain;
	}
	notavail(bufp);
	if (bufp->b_flags & B_DELWRI) {
		blk_write(bufp);
		goto tryagain;
	}
	
	bufp->b_flags = B_BUSY;
	list_add(&bufp->b_list, &devp->d_buflist);

	bufp->b_dev = dev;
	bufp->b_blkno = blkno;

found:
	return bufp;
}

/* put the buffer on the avbuflist */
void blk_release(struct buf *bufp)
{
	/* put this buf at the tail of the avbuflist */
	list_add_tail(&bufp->b_avlist, &G.avbuf_list);
}

struct buf *blk_read(dev_t dev, long blkno)
{
	struct buf *bufp;
	bufp = blk_get(dev, blkno);
	assert(bufp);
	if (bufp->b_flags & B_DONE)
		return bufp;
	bufp->b_flags |= B_READ;
	bufp->b_bcount = BLOCKSIZE;
	bdevsw[MAJOR(dev)].d_strategy(bufp);
	/* iowait(bufp); */
	return bufp;
}

void blk_write(struct buf *bufp)
{
        bufp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
        bufp->b_bcount = BLOCKSIZE;
        bdevsw[MAJOR(bufp->b_dev)].d_strategy(bufp);
        /* iowait(bufp); */
        blk_release(bufp);
}

void blk_write_delay(struct buf *bufp)
{
	bufp->b_flags |= B_DELWRI | B_DONE;
	blk_release(bufp);
}

void blk_clear(struct buf *bufp)
{
	int n = 0;
        
	if (bufp->b_flags & B_CLEAR)
		n = -1;
	
	memset(bufp->b_addr, n, BLOCKSIZE);
	bufp->b_resid = 0;
}

void blk_flush(dev_t dev)
{
	/* TODO: write this */
}

void bufinit(void)
{
	INIT_LIST_HEAD(&G.avbuf_list);
	G.numbufs = 0;
	while (G.numbufs < MINBUF)
		assert(bufalloc());
}

