/* buf.c, a simplified block i/o interface */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"

#define NUM_SECTORS 29 /* 32 - 2 (kernel) - 1 (floating) */
#define BLOCKS_PER_SECTOR 504
#define BLOCKS ((long)NUM_SECTORS * BLOCKS_PER_SECTOR)

char *blockdev = NULL; /* the virtual block device that will contain the filesystem */
unsigned long blocks;

#if 0
struct list_head buflist;
#endif

struct buf *getblk(long blkno);
struct buf *bread(long blkno);
void bwrite(struct buf *bp);
void brelse(struct buf *bp);

struct buf *getblk(long blkno)
{
        struct buf *bufp = NULL;
        struct buf *bp;
        
	if (blkno >= blocks) return NULL;
#if 0
        list_for_each_entry(bp, &buflist, b_list) {
                if (bp->b_blkno == blkno) {
                        bufp = bp;
                        break;
                }
        }
#endif
	if (!bufp) {
		bufp = malloc(sizeof(struct buf));
		if (!bufp) return NULL;
		bufp->b_blkno = blkno;
		bufp->b_addr = blockdev + (BLOCK_SIZE * blkno);
#if 0
		list_add(&bufp->b_list, &buflist);
#endif
	}
	return bufp;
}

struct buf *bread(long blkno)
{
	return getblk(blkno);
}

void bwrite(struct buf *bp)
{
	brelse(bp);
}

void brelse(struct buf *bp)
{
#if 0
	list_del(&bp->b_list);
#endif
	free(bp);
}

void bufinit(void)
{
#if 0
        INIT_LIST_HEAD(&buflist);
#endif
}
