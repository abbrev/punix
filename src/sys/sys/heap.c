#include <string.h>

#include "punix.h"
#include "heap.h"
#include "globals.h"

void *memalloc(size_t *sizep, pid_t pid)
{
	int size;
	struct heapentry *hp;
	int prevstart = 0;
	int prevsize = 0;
	
	/*
	 * TODO: if pid < 0, set *sizep to the size of the
	 * largest unallocated chunk, and return NULL
	 */
	if (G.heapsize >= HEAPSIZE || *sizep == 0 || pid < 0)
		return NULL;
	
	size = (*sizep + BLOCKSIZE - 1) / BLOCKSIZE;
	for (hp = &G.heaplist[0]; hp < &G.heaplist[G.heapsize]; ++hp) {
		if (hp->start - (prevstart + prevsize) >= size) {
			/* insert this entry here and return with pointer to start of chunk */
			memmove(hp + 1, hp,
			        sizeof(struct heapentry) * (&hp[G.heapsize] - hp)); /* XXX: is this size off by one? */
			++G.heapsize;
			hp->start = prevstart + prevsize;
			hp->size = size;
			hp->pid = pid;
			*sizep = BLOCKSIZE * size;
			return &G.heap[hp->start];
		}
		prevstart = hp->start;
		prevsize = hp->size;
	}
	return NULL;
}

void memfree(void *ptr)
{
	struct heapentry *hp;
	int start;
	if (ptr == NULL)
		return;
	start = (ptr - (void *)G.heap) / BLOCKSIZE;
	
	for (hp = &G.heaplist[0]; hp < &G.heaplist[G.heapsize]; ++hp) {
		if (start < (hp->start + hp->size)) {
			if (hp->start <= start) {
				/* remove this heap entry */
				memmove(hp, hp + 1,
				        sizeof(struct heapentry) * (&hp[G.heapsize] - hp)); /* XXX: is this size off by one? */
				--G.heapsize;
			}
			return;
		}
	}
}
