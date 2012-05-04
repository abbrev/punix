#ifndef _HEAP_H_
#define _HEAP_H_

/*
 * XXX: heapentry can be trimmed down by removing the 'end' field. Instead of
 * storing only allocations in the heap array, store the free blocks as well.
 * The size and end of each block can still be computed easily:
 * end = h[1].start;
 * size = h[1].start - h[0].start
 * This may increase the number of entries, but it may also reduce the overall
 * memory usage of the heap array. In the worst case, there will be one free
 * block for every allocated block, which means a 33% increase in overall
 * memory consumption. As long as there are only 2 free blocks for every 3
 * allocated block, or fewer, the memory usage will decrease.
 *
 * When allocating a block, a free block is located, and a new entry will need
 * to be inserted in the array only if the free block needs to be split. If the
 * free block is a perfect fit, the block simply has to be marked with the new
 * allocation's pid. If the free block is too large, and the next block is also
 * free, the difference can be subtracted from the next block's start field,
 * instead of splitting the current free block.
 *
 * When an allocated block is freed, its pid is set to "free" (currently -1, but
 * this should change to 0). Then the heap entry pointer moves down while the
 * current block is still free. Then remove the next heap entry while it is also
 * free. This merges contiguous free blocks.
 * Instead of storing two sentinels, only one at the end may be needed. This
 * sentinal block will be marked as allocated to prevent it from being merged
 * with contiguous free blocks below it.
 *
 * Merging free blocks can be deferred until a block needs to be allocated. As
 * the kernel uses only a few different sizes of blocks for its own use (eg,
 * for the buffer cache), this can save some time if blocks of the same size
 * are freed and allocated repeatedly.
 *
 * For example, assume four contiguous blocks are allocated. The middle two
 * blocks are then freed, leaving two contiguous free blocks in the hole. Then
 * another block (same size as one of the free blocks) is allocated from one of
 * the free blocks. All that needs to take place in this scenario is a change
 * in the pid field in one of the free blocks. The other free block still
 * reflects how much space is free between the blocks. If, on the other hand,
 * the free blocks were merged when the two middle blocks were freed, the free
 * block would have to be split in two again to allocate a block from it.
 *
 * Compared to the current heap implementation, this deferred free-block merging
 * should reduce the number of heap shifting that would be needed.
 *
 * As another bonus, the heapentry structure would become a power of two in
 * size (four bytes), so it might be better optimized at the machine code level.
 */
struct heapentry {
	short start;
	short end;
	pid_t pid;
};

void meminit();
void *memalloc(size_t *sizep, pid_t pid);
void memfree(void *ptr, pid_t pid);
void memfreepid(pid_t pid);
void *memrealloc(void *ptr, size_t *newsizep, int direction, pid_t pid);
size_t heap_get_total();
size_t heap_get_used();
size_t heap_get_free();

#define MEMREALLOC_BOTTOM (-1)
#define MEMREALLOC_AUTO   0
#define MEMREALLOC_TOP    1


#endif
