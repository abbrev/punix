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

#define SECTORSIZE 0x10000L
#define NUMSECTORS 32
#define FLASH      ((flash_t)0x200000)
#define FLASHSIZE  (SECTORSIZE * NUMSECTORS)
#define FLASHEND   (&FLASH[NUMSECTORS])

#define FBFREE  (-1) /* free */
#define FBUSED  (-2) /* used */
#define FBDEL   (-4) /* deleted but still open */
#define FBDIRTY (-8) /* dirty (obsoleted) */

extern long __ld_program_size;
#define ARCHIVE  ((flash_t)&((char *)FLASH)[(__ld_program_size + 0xffff) % 0x10000])
#define ARCEND   FLASHEND

typedef char (*flash_t)[SECTORSIZE];

/*
 * TO-DO:
 * 
 * o Make a LRU mapping of block numbers to flashrom addresses. This would make
 *   accessing a recently-accessed block faster.
 * o Should flopen() allow device to be opened once?
 */

#define CACHE_SIZE 32

struct cache_entry {
	long blkno;
	struct flashblock *fbp;
};

static struct cache_entry cache[CACHE_SIZE];

/* make this cache entry the most recently used entry */
static void makeru(struct cache_entry *cep)
{
	struct cache_entry ce;
	
	if (cep == &cache[0])
		return;
	ce = *cep;
	memmove(&cache[1], &cache[0], (size_t)((void *)cep - (void *)&cache[0]));
	cache[0] = ce;
}

static struct cache_entry *cachefind(long blkno)
{
	struct cache_entry *cep;
	
	/* search for the block */
	for (cep = &cache[0]; cep < &cache[CACHE_SIZE]; ++cep) {
		if (cep->blkno == blkno) {
			makeru(cep);
			return cep;
		}
	}
	return NULL;
}

static void cacheadd(long blkno, struct flashblock *fbp)
{
	struct cache_entry *cep;
	
	if (cachefind(blkno))
		return;
	
	/* we couldn't find this cache entry, so add it to the end */
	cep = &cache[CACHE_SIZE-1];
	cep->blkno = blkno;
	cep->fbp = fbp;
	makeru(cep);
}

#if 0
/* cache using a modified "clock" page replacement algorithm */

#define CACHE_SIZE 32
#define CACHE_REF (1 << 15) /* cache reference bit */

struct cache_entry {
	unsigned int ref;
	long blkno;
	struct flashblock *fbp;
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

/* Find the block numbered 'blkno' and return a pointer to the flashblock.
 * Return NULL if this block number doesn't exist.
 */
STARTUP(static struct flashblock *getfblk(long blkno))
{
	int sector;
	struct flashblock *fbp;
	struct cache_entry *cep;
	
	if ((cep = cachefind(blkno)))
		    return cep->fbp;
	
	for (sector = 0; &ARCHIVE[sector] < ARCEND; ++sector) {
		for (fbp = (struct flashblock *)&ARCHIVE[sector];
		     (fbp+1) < (struct flashblock *)ARCHIVE[sector+1]; /* the whole block must fit in the sector */
		     ++fbp) {
			if (fbp->status == FBFREE) /* end of this sector */
				break;
			if ((fbp->status == FBUSED || fbp->status == FBDEL)
			    && fbp->blockno == blkno) {
				goto out;
			}
		}
	}
	
	fbp = NULL;
out:
	cacheadd(blkno, fbp);
	return fbp;
}

extern char *bufalloc();

static void makecopy(struct buf *bp)
{
	char *cp;
	if (bp->b_flags & B_COPY) return;
	
	cp = bufalloc();
	memcpy(cp, bp->b_addr, BLOCKSIZE);
	bp->b_addr = cp;
	bp->b_flags |= B_COPY;
}

/* FIXME: advance the currentfblock pointer to the next free one */
static void nextblock()
{
	++G.currentfblock;
}

STARTUP(static void flread(struct buf *bp))
{
	struct flashblock *fbp = getfblk(bp->b_blkno);
	
	bp->b_flags |= B_DONE;
	
	if (fbp) {
		bp->b_addr = fbp->data;
		bp->b_flags &= ~B_COPY;
		if (bp->b_flags & B_WRITABLE) {
			makecopy(bp);
		}
	} else {
		bp->b_flags &= ~B_WRITABLE;
		clrbuf(bp);
	}
}

STARTUP(static void flwrite(struct buf *bp))
{
	char *startdiff, *enddiff, *diffp;
	int newblock = bp->b_flags & B_NEWB;
	char *cp;
	struct flashblock *oldfbp, *newfbp;
	char *writep;
	int len;
	struct flashblock *fbp = getfblk(bp->b_blkno);
	
	if (!(bp->b_flags & B_COPY)) {
		if (bp->b_flags & B_NEWB) {
			bp->b_flags |= B_WRITABLE;
			flread(bp);
		} else
			return;
	}
	
	oldfbp = getfblk(bp->b_blkno);
	if (oldfbp) {
		cp = oldfbp->data;
		/* find the first byte that is different */
		for (startdiff = fbp->data; !newblock && startdiff < &fbp->data[BLOCKSIZE]; ++startdiff, ++cp) {
			if (*cp == *startdiff) continue;
			if ((*cp & *startdiff) != *startdiff)
				newblock = 1;
			break;
		}
		/* find the last byte that is different */
		for (diffp = enddiff = startdiff; !newblock && diffp < &fbp->data[BLOCKSIZE]; ++diffp, ++cp) {
			if (*cp == *diffp) continue;
			enddiff = diffp;
			if ((*cp & *diffp) != *diffp)
				newblock = 1;
		}
	} else {
		newblock = 1;
	}
	
	if (newblock) {
		startdiff = fbp->data;
		enddiff = &fbp->data[BLOCKSIZE - 1];
		newfbp = G.currentfblock;
	} else {
		newfbp = oldfbp;
	}
	writep = newfbp->data + (startdiff - fbp->data);
	len = enddiff - startdiff + 1;
	
	FlashWrite(startdiff, writep, len);
	if (newblock) {
		/*
		 * write the new flash block header
		 * obsolete the old flash block
		 */
		nextfblock();
	}
	cacheadd(fbp->blockno, fbp);
}

STARTUP(static void fldelete(struct buf *bp))
{
	unsigned short status;
	struct flashblock *fbp = getfblk(bp->b_blkno);
	if (!fbp)
		return;
	
	if (bp->b_flags & B_FREE) {
		status = FBDIRTY; /* completely delete this block */
	} else {
		status = FBDEL; /* keep block around until flash closes */
	}
	
	FlashWrite(&fbp->status, &status, sizeof(status));
	cacheadd(fbp->blockno, NULL);
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
		fldelete(bp);
	} else if (bp->b_flags & B_READ) {
		flread(bp);
	} else if (bp->b_flags & B_WRITE) {
		flwrite(bp);
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
