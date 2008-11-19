#include <string.h>
#include <errno.h>

#include "punix.h"
#include "heap.h"
#include "globals.h"

static void printheaplist()
{
	int i;
	for (i = 0; i < G.heapsize; ++i) {
		kprintf("%5d: %5d %5d %5d (0x%06lx)\n", i, (int)G.heaplist[i].pid, (int)G.heaplist[i].start, (int)G.heaplist[i].end, (void *)&G.heap[G.heaplist[i].start]);
	}
}

static size_t largest_unallocated_chunk_size()
{
	int size = 0;
	int prevend = 0;
	int s;
	struct heapentry *hp;
	for (hp = &G.heaplist[1]; hp < &G.heaplist[G.heapsize]; ++hp) {
		s = hp->start - prevend;
		if (size < s)
			size = s;
		prevend = hp->end;
	}
	return (size_t)size * HEAPBLOCKSIZE;
}

void meminit()
{
	struct heapentry *hp = &G.heaplist[0];
	/* insert two sentinal heap entries,
	 * one at the bottom and one at the top of the heap */
	hp->start = 0;
	hp->end = 0;
	hp->pid = -1;
	++hp;
	hp->start = ((void *)0x40000 - (void *)G.heap[0]) / HEAPBLOCKSIZE;
	hp->end = 0;
	hp->pid = -1;
	G.heapsize = 2;
	
#if 1
	struct var {
		size_t size;
		char *name;
	} var[50];
	int numvars = 0;
#define ADDVAR(n) do { var[numvars].size = sizeof(G.n); var[numvars++].name = #n; } while (0)
	ADDVAR(exec_ram);
	ADDVAR(_walltime);
	ADDVAR(_runrun);
	ADDVAR(_istick);
	ADDVAR(_ioport);
	ADDVAR(_cputime);
	ADDVAR(_updlock);
	ADDVAR(_current);
	ADDVAR(lowestpri);
	ADDVAR(freeproc);
	ADDVAR(proc);
	ADDVAR(file);
	ADDVAR(loadavg);
	ADDVAR(audiosamp);
	ADDVAR(audiosamples);
	ADDVAR(audiolowat);
	ADDVAR(audioplay);
	ADDVAR(audiooptr);
	
	ADDVAR(linklowat);
	
	ADDVAR(linkreadq);
	ADDVAR(linkwriteq);
	ADDVAR(audioq);
	
	ADDVAR(prngseed);
	
	ADDVAR(rootdev);
	ADDVAR(pipedev);
	ADDVAR(rootdir);
	
	ADDVAR(canonb);
	ADDVAR(inode);
	ADDVAR(mpid);
	ADDVAR(pidchecked);
	ADDVAR(callout);
	
	ADDVAR(avbuflist);
	ADDVAR(buffers);
	
	ADDVAR(currentfblock);
	ADDVAR(flash_cache);
	
	ADDVAR(contrast);
	ADDVAR(vt);
	
	ADDVAR(batt_level);
	
	ADDVAR(heapsize);
	ADDVAR(heaplist);
	ADDVAR(heap);
	/* sort */
	int i, j;
	struct var t;
	for (i = 0; i < numvars - 1; ++i) {
		for (j = i; j >= 0; --j) {
			if (var[j].size >= var[j+1].size) break; /* in order */
			t = var[j];
			var[j] = var[j+1];
			var[j+1] = t;
		}
	}
	size_t totalsize = 0;
	for (i = 0; i < numvars; ++i)
		totalsize += var[i].size;
	kprintf("totalsize = %ld\n", totalsize);
	for (i = 0; i < numvars; ++i)
		kprintf("%2ld.%02ld%% %-13.13s", 100 * var[i].size / totalsize, (10000 * var[i].size / totalsize) % 100, var[i].name);
	kputchar('\n');
	kprintf("%5ld largest unallocated chunk size\n", largest_unallocated_chunk_size());
	size_t size;
	void *p;
	size_t s = (size_t)128;
	for (i = 0; i < 32000; ++i) {
		p = memalloc(&s, 0);
		if (p)
			memfree(p, 0);
	}
	printheaplist();
	/* abort(); */
#endif
}

/* insert an entry before existing entry hp, moving hp and other entries up */
static void insertentry(struct heapentry *hp, int start, int end, pid_t pid)
{
	memmove(hp + 1, hp, (void *)&hp[G.heapsize] - (void *)hp);
	++G.heapsize;
	
	hp->start = start;
	hp->end = end;
	hp->pid = pid;
}

/*
 * sizep is a pointer to the desired size. The actual size of the allocated
 * chunk is returned in the destination of sizep.
 * 
 * pid is the process id of the process that owns this memory chunk.
 * If pid < 0, memalloc returns the size of the largest unallocated chunk.
 */
void *memalloc(size_t *sizep, pid_t pid)
{
	int size;
	struct heapentry *hp;
	
	if (!sizep) return NULL;
	
	if (pid < 0) {
		*sizep = largest_unallocated_chunk_size();
		return NULL;
	}
	
	if (G.heapsize >= HEAPSIZE) return NULL;
	
	/*
	 * If *sizep is either 0 or is large enough that
	 * (*sizep + HEAPBLOCKSIZE - 1) wraps around, size will be 0,
	 * and the following test will catch this.
	 */
	size = (*sizep + HEAPBLOCKSIZE - 1) / HEAPBLOCKSIZE;
	if (size == 0)
		return NULL;
	
#define SIZETHRESHOLD 2048
	if (size < SIZETHRESHOLD / HEAPBLOCKSIZE) {
		int prevstart = G.heaplist[G.heapsize-1].start;
		for (hp = &G.heaplist[G.heapsize-2]; hp >= &G.heaplist[0]; --hp) {
			if (size <= (prevstart - hp->end)) {
				++hp;
				/*
				 * insert this entry here and return
				 * with a pointer to the start of the chunk
				 */
				insertentry(hp, prevstart - size, prevstart, pid);
				*sizep = (size_t)HEAPBLOCKSIZE * size;
				return &G.heap[hp->start];
			}
			prevstart = hp->start;
		}
	}
	
	int prevend = 0;
	for (hp = &G.heaplist[1]; hp < &G.heaplist[G.heapsize]; ++hp) {
		if (size <= hp->start - prevend) {
			/*
			 * insert this entry here and return
			 * with a pointer to the start of the chunk
			 */
			insertentry(hp, prevend, prevend + size, pid);
			*sizep = (size_t)HEAPBLOCKSIZE * size;
			return &G.heap[hp->start];
		}
		prevend = hp->end;
	}
	return NULL;
}

static void removeentry(struct heapentry *hp)
{
	memmove(hp, hp + 1, (void *)&hp[G.heapsize] - (void *)hp);
	--G.heapsize;
}

#if 0
/* linear search method */
void memfree(void *ptr, pid_t pid)
{
	struct heapentry *hp;
	int start;
	if (ptr == NULL)
		return;
	start = (ptr - (void *)G.heap) / HEAPBLOCKSIZE;
	
	for (hp = &G.heaplist[0]; hp < &G.heaplist[G.heapsize]; ++hp) {
		if (start < hp->end) {
			if (hp->start <= start) {
				/* remove this heap entry */
				if (!pid || pid == hp->pid)
					removeentry(hp);
				else
					P.p_error = EFAULT;
			}
			return;
		}
	}
}
#else
/* binary search method */
void memfree(void *ptr, pid_t pid)
{
	struct heapentry *hp;
	int start;
	int lower, middle, upper;
	if (ptr == NULL)
		return;
	start = (ptr - (void *)G.heap) / HEAPBLOCKSIZE;
	lower = 1;
	upper = G.heapsize - 1;
	
	do {
		middle = (lower + upper) / 2;
		hp = &G.heaplist[middle];
		if (start < hp->start) {
			upper = middle;
			continue;
		} else if (start >= hp->end) {
			lower = middle + 1;
			continue;
		}
		/* remove this heap entry */
		if (!pid || pid == hp->pid)
			removeentry(hp);
		else
			P.p_error = EFAULT;
		break;
	} while (lower < upper);
}
#endif

void sys_memalloc()
{
	struct a {
		size_t *sizep;
	} *ap = (struct a *)P.p_arg;
	
	if (ap->sizep)
		P.p_retval = (unsigned long)memalloc(ap->sizep, P.p_pid);
	else
		P.p_error = EINVAL;
}

void sys_memfree()
{
	struct a {
		void *ptr;
	} *ap = (struct a *)P.p_arg;
	
	if (ap->ptr)
		memfree(ap->ptr, P.p_pid);
}
