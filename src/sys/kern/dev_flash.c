/*
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
*/

#include "punix.h"
#include "buf.h"
#include "dev.h"

#define SECTORSIZE 32768
#define NUMSECTORS 32

#define FBFREE  (-1)
#define FBUSED  (-2)
#define FBDIRTY (-3) /* or deleted */

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

/* BLOCKSIZE and LOGSIZE are the size of the FS block and log, respectively, in
 * 2-byte (WORD) units */
#define LOGSIZE (BLOCKSIZE + sizeof(struct flashblock)/2)

const struct devtab fltab;

/* extern short archive[][SECTORSIZE]; */

struct flashblock {
	short status;
	long blockno;
} __attribute__((packed));

/* Find the block numbered 'blkno' and return a pointer to the data portion.
 * Return NULL if this block number doesn't exist.
 */
STARTUP(static struct flashblock *getfblk(long blkno))
{
	int page, block;
	struct flashblock *fbp;
	
	/* search for the block. */
	/* FIXME: cache the blkno=>addr mapping for faster future reference. */
	for (page = 0; page < NUMSECTORS; ++page) {
		for (block = 0; block < SECTORSIZE/LOGSIZE; ++block) {
			fbp = (struct flashblock *)&archive[page*SECTORSIZE +
			 block*LOGSIZE];
			
			if (fbp->status == FBUSED && fbp->blockno == blkno)
					return fbp + 1;
		}
	}
	return NULL;
}

STARTUP(static char *findunused(long blkno))
{
	/* FIXME: find an unused slot for a block and set its status and block
	 * number */
	return NULL;
}

STARTUP(static void flread(char *addr, struct buf *bp))
{
	bp->b_flags |= B_DONE;
	
	if (addr) {
		bp->b_flags &= ~B_COPY; /* this is not a copy */
		bp->b_addr = addr;
	} else {
		bp->b_flags |= B_COPY; /* this is a copy */
		/* FIXME: zero out the buffer */
	}
}

STARTUP(static void flwrite(char *addr, struct buf *bp))
{
	char *newaddr = NULL;
	/* this is where it gets interesting... */
	
	/* we need to find a free block entry, hopefully in a sector with the
	 * least number of dirty blocks */
	
	if (addr) {
		if (!(bp->b_flags & B_COPY)) /* it's already in flash! */
			return;
		if (bp->b_flags & B_WIP) /* write this block in place */
			newaddr = addr;
	}
	
	if (!newaddr && !(newaddr = findunused(bp->b_blkno)))
		return;
	
	FlashWrite(bp->b_addr, newaddr, BLOCKSIZE);
}

STARTUP(static void fldelete(char *addr, struct buf *bp))
{
	if (!addr)
		return;
	
	/* FIXME: delete this block by changing its status, but only if it is
	 * used and the block number is the same. */
}

STARTUP(void flopen(struct file *fp))
{
	/* XXX: flush the cache? */
}

STARTUP(void flclose(struct file *fp))
{
}

STARTUP(void flstrategy(struct buf *bp))
{
	struct flashblock *fbp;
	
	fbp = getfblk(bp->b_blkno);
	
	if (bp->b_flags & B_READ) {
		flread((char *)fbp, bp);
	} else if (bp->b_flags & B_WRITE) {
		flwrite((char *)fbp, bp);
	} else if (bp->b_flags & B_DELETE) {
		fldelete((char *)fbp, bp);
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

/* size is in bytes */
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
	
	size = (size + 1) / 2;
	
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
