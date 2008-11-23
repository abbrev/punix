#ifndef _BUF_H_
#define _BUF_H_

#define	B_WRITE       0	/* non-read pseudo-flag */
#define	B_READ       01	/* read when I/O occurs */
#define	B_DONE       02	/* transaction finished */
#define	B_ERROR      04	/* transaction aborted */
#define	B_BUSY      010	/* not on av_forw/back list */
#define	B_PHYS      020	/* Physical IO potentially using UNIBUS map */
#define	B_MAP       040	/* This block has the UNIBUS map allocated */
#define	B_WANTED   0100	/* issue wakeup when BUSY goes off */
#define	B_RELOC    0200	/* no longer used */
#define	B_ASYNC    0400	/* don't wait for I/O completion */
#define	B_DELWRI  01000	/* don't write till block leaves available list */

/* clean up these #defines */
#define	B_COPY    02000	/* this is a copy of the flash block */
#define B_DELETE  04000	/* mark this block as "deleted" but in use */
#define B_FREE   010000 /* free this flash block */
#define B_NEWB   020000	/* write this block to a new flash block */
#define B_CLEAR  040000 /* clear natively (set all bits for flash) */
#define B_WRITABLE 0100000

/*
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * the device with which it is currently associated (always)
 * and also on a list of blocks available for allocation
 * for other use (usually).
 * The latter list is kept in last-used order, and the two
 * lists are doubly linked to make it easy to remove
 * a buffer from one list when it was found by
 * looking through the other.
 * A buffer is on the available list, and is liable
 * to be reassigned to another disk block, if and only
 * if it is not marked BUSY.  When a buffer is busy, the
 * available-list pointers can be used for other purposes.
 * Most drivers use the forward ptr as a link in their I/O
 * active queue.
 * A buffer header contains all the information required
 * to perform I/O.
 * Most of the routines which manipulate these things
 * are in bio.c.
 */
struct buf {
	struct buf *b_next, *b_prev;
	struct buf *b_avnext, *b_avprev;
	int b_flags;
	dev_t b_dev;
	size_t b_bcount;
	void *b_addr;
	long b_blkno;
	int b_error;
	size_t b_resid; /* bytes not transferred after error */
};

/*
 * Each block device has a devtab, which contains private state stuff
 * and 2 list heads: the b_forw/b_back list, which is doubly linked
 * and has all the buffers currently associated with that major
 * device; and the d_actf/d_actl list, which is private to the
 * device but in fact is always used for the head and tail
 * of the I/O queue for the device.
 * Various routines in bio.c look at b_forw/b_back
 * (notice they are the same as in the buf structure)
 * but the rest is private to each device driver.
 */
struct devtab
{
	struct	buf *b_next;		/* first buffer for this dev */
	struct	buf *b_prev;		/* last buffer for this dev */
#if 0
	struct	buf *d_actf;		/* head of I/O queue */
	struct 	buf *d_actl;		/* tail of I/O queue */
	char	d_active;		/* busy flag */
	char	d_errcnt;		/* error count (for recovery) */
#endif
};

void brelse(struct buf *bp);
struct buf *getblk(dev_t dev, long blkno);
struct buf *bread(dev_t dev, long blkno);

#endif
