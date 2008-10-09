#ifndef _HEAP_H_
#define _HEAP_H_

struct heapentry {
	short start;
	short size;
	pid_t pid;
};

void *memalloc(size_t *sizep, pid_t pid);
void memfree(void *ptr);

#endif
