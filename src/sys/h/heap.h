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

#endif
