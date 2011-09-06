#ifndef _HEAP_H_
#define _HEAP_H_

struct heapentry {
	short start;
	short end;
	pid_t pid;
};

void meminit();
void *memalloc(size_t *sizep, pid_t pid);
void memfree(void *ptr, pid_t pid);
void *memrealloc(void *ptr, size_t *newsizep, int direction, pid_t pid);
size_t heap_get_total();
size_t heap_get_used();
size_t heap_get_free();

#define MEMREALLOC_BOTTOM (-1)
#define MEMREALLOC_AUTO   0
#define MEMREALLOC_TOP    1


#endif
