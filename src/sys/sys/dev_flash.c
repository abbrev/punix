/*
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
*/
#include <string.h>

#include "punix.h"
#include "buf.h"
#include "flash.h"
#include "dev.h"
#include "globals.h"

#define SECTORSIZE 32768
#define NUMSECTORS 32

#define FBFREE  (-1)
#define FBUSED  (-2)
#define FBDEL   (-4) /* deleted but still open */
#define FBDIRTY (-8) /* obsolete */

extern long __ld_program_size;

typedef short archive_t[][SECTORSIZE];

#if 0
extern archive_t archive;
#else
#define archive (*(archive_t *)((__ld_program_size+0x200000 + 0xffff) & 0xffff))
#endif

/*
 * TO-DO:
 * 
 * o Make a LRU mapping of block numbers to flashrom addresses. This would make
 *   accessing a recently-accessed block faster.
 * o Should flopen() allow device to be opened once?
 * o Write flwrite().
 */

#if 0
/* cache using a modified "clock" page replacement algorithm */

#define CACHE_SIZE 32
#define CACHE_REF (1 << 15) /* cache reference bit */

struct cache_entry {
	unsigned int ref;
	long blkno;
	void *addr;
};

/* traversing the cache is cheaper than searching (on average) 8192 blocks for
 * a particular block number in Flash ROM */

struct cache_entry cache[CACHE_SIZE];
struct cache_entry *hand;

/* to find a block's physical address: */
void *blkno_to_addr(long blkno)
{
	struct cache_entry *cp;
	void *addr;
	
	/* see if the block is in cache */
	for (cp = &cache[0]; cp < &cache[CACHE_SIZE]; ++cp) {
		if (cp->blkno == blkno) {
			cp->ref |= CACHE_REF;
			return cp->addr; 
		}
	}
	
	/* we couldn't find this block in the cache (cache miss) */
	
	/* search the flash for the block */
	/* addr = ???; */
	
	/* find a victim cache entry to put this new one */
	struct cache_entry *hand_start = hand, *best = hand;
	while (hand->ref) {
		hand->ref >>= 1;
		++hand;
		if (hand >= &cache[CACHE_SIZE])
			hand = &cache[0];
		cp = hand;
		if (hand->ref < best->ref)
			best->ref = hand;
		if (hand_start == hand) {
			cp = best;
			break;
		}
	}
	/* cp points to the victim cache entry */
	cp->ref = CACHE_REF;
	cp->blkno = blkno;
	cp->addr = addr;
	return cp;
}
#endif

short FlashWrite(const void *src asm("%a2"), void *dest asm("%a3"),
                 size_t size asm("%d3"));
short FlashErase(const void *dest asm("%a2"));

const struct devtab fltab;

/* extern short archive[][SECTORSIZE]; */

/* XXX: the '- 1' at the end is because sectors are not evenly divisible by
 * flash blocks */
#define BLOCKSPERSECTOR (2 * SECTORSIZE / sizeof(struct flashblock) - 1)

/* Find the block numbered 'blkno' and return a pointer to the flashblock.
 * Return NULL if this block number doesn't exist.
 */
STARTUP(static struct flashblock *getfblk(long blkno))
{
	int sector, block;
	struct flashblock *fbp;
	
	/* search for the block. */
	/* FIXME: cache the blkno=>addr mapping for faster future reference. */
	for (sector = 0; sector < NUMSECTORS; ++sector) {
		for (block = 0; block < BLOCKSPERSECTOR; ++block) {
			fbp = (struct flashblock *)&archive[sector][block*sizeof(struct flashblock)/2];
			
			if (fbp->status == FBUSED && fbp->blockno == blkno)
					return fbp;
		}
	}
	return NULL;
}

STARTUP(static void flread(struct flashblock *fbp, struct buf *bp))
{
	bp->b_flags |= B_DONE;
	
	if (fbp) {
		bp->b_flags &= ~B_COPY; /* this is not a copy */
		bp->b_addr = fbp->data;
	} else {
		int n = 0;
		bp->b_flags |= B_COPY; /* this is a copy */
		/* FIXME: allocate a block buffer */
		
		/* clear the buffer according to the bit B_CLEAR */
		if (bp->b_flags & B_CLEAR)
			n = -1; /* flash native clear state */
		memset(fbp->data, n, BLOCKSIZE);
	}
}

STARTUP(static void flwrite(struct flashblock *fbp, struct buf *bp))
{
	unsigned short *startdiff, *enddiff, *diffp;
	int newblock = bp->b_flags & B_NEWB;
	unsigned short *cp;
	struct flashblock *cfbp, *newfbp;
	unsigned short *writep;
	int len;
	
	if (!(bp->b_flags & B_COPY)) return;
	
	cfbp = getfblk(bp->b_blkno);
	if (cfbp) {
		cp = cfbp->data;
		/* find the first byte that is different */
		for (startdiff = fbp->data; !newblock && startdiff < &fbp->data[BLOCKSIZE/2]; ++startdiff, ++cp) {
			if (*cp == *startdiff) continue;
			if ((*cp & *startdiff) != *startdiff)
				newblock = 1;
			break;
		}
		/* find the last byte that is different */
		for (diffp = enddiff = startdiff; !newblock && diffp < &fbp->data[BLOCKSIZE/2]; ++diffp, ++cp) {
			if (*cp == *diffp) continue;
			enddiff = diffp;
			if ((*cp & *diffp) != *diffp)
				newblock = 1;
		}
	} else {
		newblock = 1;
		newfbp = G.currentfblock;
	}
	if (newblock) {
		startdiff = fbp->data;
		enddiff = &fbp->data[BLOCKSIZE / 2 - 1];
		newfbp = G.currentfblock;
	} else {
		newfbp = cfbp;
	}
	writep = newfbp->data + (startdiff - fbp->data);
	len = enddiff - startdiff + 1;
	
	FlashWrite(startdiff, writep, len);
	if (newblock) {
		/*
		 * write the new flash block header
		 * obsolete the old flash block
		 * update G.currentfblock
		 */
	}
}

STARTUP(static void fldelete(struct flashblock *fbp, struct buf *bp))
{
	unsigned short status;
	if (!fbp)
		return;
	
	if (bp->b_flags & B_FREE) {
		status = FBDIRTY; /* completely delete this block */
	} else {
		status = FBDEL; /* keep block around until flash closes */
	}
	
	FlashWrite(&fbp->status, &status, sizeof(status));
}

STARTUP(void flopen(dev_t dev))
{
	/* XXX: flush the cache? */
	/* FIXME: init the cache and G.currentfblock */
}

STARTUP(void flclose(dev_t dev))
{
}

STARTUP(void flstrategy(struct buf *bp))
{
	struct flashblock *fbp;
	
	fbp = getfblk(bp->b_blkno);
	
	if ((bp->b_flags & B_DELETE) || (bp->b_flags & B_FREE)) {
		fldelete(fbp, bp);
	} else if (bp->b_flags & B_READ) {
		flread(fbp, bp);
	} else if (bp->b_flags & B_WRITE) {
		flwrite(fbp, bp);
	}
}

#if 0
/* FIXME: rewrite these for real hardware! */
/* this works */
int FlashInit(FILE *flashfile)
{
	long i;
	
	for (i = 0; i < NUMSECTORS; ++i)
		FlashErase(&flashrom[i * SECTORSIZE]);
	
	if (flashfile) {
		int x;
		for (i = 0; i < FLASHSIZE; ++i) {
			fscanf(flashfile, "%x", &x);
			flashrom[i] = x;
		}
	}
	
	return 0;
}

/* size is in words (2 bytes) */
STARTUP(int FlashWrite(const short *src, short *dest, size_t size))
{
	int sector1, sector2;
	off_t offset;
	size_t i;
	short *wdest;
	
	/* check batteries */
	/* unprotect flash */
	/* check that src is in RAM */
	
	/* check that dest is word-aligned */
	offset = (const char *)dest - (const char *)&flashrom[0][0];
	if (offset & 1)
		return 1;
	
	/* check that dest is in ROM */
	if (dest < &flashrom[0][0] || &flashrom[NUMSECTORS][0] <= dest)
		return 1;
	
	/* check that the block starts and ends in the same sector */
	sector1 = (dest - &flashrom[0][0]) / SECTORSIZE;
	sector2 = (dest + size - 1 - &flashrom[0][0]) / SECTORSIZE;
	if (sector1 != sector2)
		return 1;
	
	/* copy src to dest */
	wdest = (short *)dest;
	for (i = 0; i < size; ++i) {
		if ((*dest & *src) != *src)
			/*fprintf(stderr, "bits are lost (%p)\n", dest)*/;
		
		*wdest++ &= *src++;
	}
	
	/* protect flash */
	
	return 0;
}

/* this works */
STARTUP(int FlashErase(const short *dest))
{
	int sector;
	int i;
	short *wdest;
	
	/* check batteries */
	/* unprotect flash */
	
	/* check that dest is in ROM */
	if (dest < &flashrom[0][0] || &flashrom[NUMSECTORS][0] <= dest)
		return 1;
	
	/* round to sector size */
	sector = (dest - &flashrom[0][0]) / SECTORSIZE;
	
	/* clear sector (set to all 1 bits) */
	for (wdest = (short *)&flashrom[SECTORSIZE * sector], i = SECTORSIZE;
			i; --i)
		*wdest++ = 0xFFFF;
	
	return 0;
}
#endif
