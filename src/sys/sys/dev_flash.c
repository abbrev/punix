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

#if 0
extern long __ld_program_size;
#define ARCHIVE  ((flash_t)&((char *)FLASH)[(__ld_program_size + 0xffff) % 0x10000])
#else
#define FIRSTSECTOR 3 /* should be enough */
#define ARCHIVE (&FLASH[FIRSTSECTOR])
#endif
#define ARCEND   FLASHEND

typedef char (*flash_t)[SECTORSIZE];

/*
 * TO-DO:
 * 
 * o Make a LRU mapping of block numbers to flashrom addresses. This would make
 *   accessing a recently-accessed block faster.
 * o Should flopen() allow device to be opened once?
 */

/* make this cache entry the most recently used entry */
static void makeru(struct flash_cache_entry *cep)
{
	struct flash_cache_entry ce;
	
	if (cep == &G.flash.flash_cache[0])
		return;
	ce = *cep;
	memmove(&G.flash.flash_cache[1], &G.flash.flash_cache[0], (size_t)((void *)cep - (void *)&G.flash.flash_cache[0]));
	G.flash.flash_cache[0] = ce;
}

static struct flash_cache_entry *cachefind(long blkno)
{
	struct flash_cache_entry *cep;
	
	/* search for the block */
	for (cep = &G.flash.flash_cache[0]; cep < &G.flash.flash_cache[FLASH_CACHE_SIZE]; ++cep) {
		if (cep->blkno == blkno) {
			makeru(cep);
			return cep;
		}
	}
	return NULL;
}

static void cacheadd(long blkno, struct flashblock *fbp)
{
	struct flash_cache_entry *cep;
	
	if (cachefind(blkno))
		return;
	
	/* we couldn't find this cache entry, so add it to the end */
	cep = &G.flash.flash_cache[FLASH_CACHE_SIZE-1];
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

const struct devtab fltab;

void flashinit(void)
{
	/* TODO: initialize the flash device driver */
	
	return;
	
	int i;
	char test[] = "hello";
	/* run a few tests on the Flash{Write,Erase} routines */
	kprintf("flashinit(): Running a few tests on the Flash routines...\n");
	kprintf("testing FlashWrite() and FlashErase(): ");
	for (i = 0; i < 100; ++i) {
		FlashWrite(test, ARCHIVE, sizeof(test));
		FlashErase(ARCHIVE);
	}
	kprintf("done\n");
}

#define MAXBLKNO 15000L /* XXX: this needs to be calculated based on the actual number of blocks available */

/* Find the block numbered 'blkno' and return a pointer to the flashblock.
 * Return NULL if this block number doesn't exist.
 */
STARTUP(static struct flashblock *getfblk(long blkno))
{
	int sector;
	struct flashblock *fbp;
	struct flash_cache_entry *cep;
	
	if (blkno < 0 || MAXBLKNO <= blkno)
		return NULL;
	if ((cep = cachefind(blkno)))
		    return cep->fbp;
	
	for (sector = 0; &ARCHIVE[sector] < ARCEND; ++sector) {
		for (fbp = (struct flashblock *)&ARCHIVE[sector];
		     (fbp+1) < (struct flashblock *)ARCHIVE[sector+1]; /* the whole block must fit in the sector */
		     ++fbp) {
			if (fbp->blockno == ~0) /* end of this sector */
				break;
			if (fbp->blockno == blkno) {
				goto out;
			}
		}
	}
	
	fbp = NULL;
out:
	cacheadd(blkno, fbp);
	return fbp;
}

/* FIXME: advance the currentfblock pointer to the next free one */
static void nextblock()
{
	++G.flash.currentfblock;
}

STARTUP(static void flread(struct buf *bp))
{
	struct flashblock *fbp = getfblk(bp->b_blkno);
	
	bp->b_flags |= B_DONE;
	
	if (fbp) {
		memcpy(bp->b_addr, fbp->data, BLOCKSIZE);
	} else {
		blk_clear(bp);
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
		newfbp = G.flash.currentfblock;
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
	long zero = 0;
	struct flashblock *fbp = getfblk(bp->b_blkno);
	if (!fbp)
		return;
	
	FlashWrite(&fbp->blockno, &zero, sizeof(zero));
	cacheadd(fbp->blockno, NULL);
}

STARTUP(void flopen(dev_t dev))
{
	/* XXX: flush the cache? */
	/* FIXME: init the cache and G.flash.currentfblock */
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
