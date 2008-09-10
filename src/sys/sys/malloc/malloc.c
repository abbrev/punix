/* FIXME: massage this file please */
/* make a struct with self-pointer "next" and size_t "size".
 * also, add process owner information (links/whatever) to the struct.
 */

#include <stdio.h>
#include <stdlib.h>

struct heap_block {
	struct heap_block *next;
	size_t size;
};

unsigned char *heap;

/* heapfree is a list of free blocks in the heap. */
struct heap_block heapfree;

void heap_init(void)
{
	fprintf(stderr, "heap_init {");
	heap = malloc(192*1024);
	if (!heap)
		abort();

	((struct heap_block *)heap)->next = NULL;
	((struct heap_block *)heap)->size = 192*1024-sizeof(struct heap_block);
	heapfree.next = (struct heap_block *)heap;
	heapfree.size = 0;
	fprintf(stderr, "}");
}

void *kmalloc(size_t size)
{
	struct heap_block *prev;
	struct heap_block *fp; /* free block pointer */

	fprintf(stderr, "kmalloc {");

	if (!size) {
		fprintf(stderr, "}");
		return NULL;
	}

#if 0
	size += sizeof(struct heap_block);      /* add headersize */
#endif
	size = (size + 3) & ~3;	/* round up to 4-byte boundary */

	fprintf(stderr, "(size = %ld) ", size);

	/* FIXME: if size overflows, return 0 */

	prev = &heapfree;

	for (fp = heapfree.next; fp; prev = fp, fp = fp->next) {
		fprintf(stderr, "a");
		if (fp->size >= size) {
			fprintf(stderr, "(found slot large enough) ");
			if (fp->size == size) {
				prev->next = fp->next;
				/*--HeapBlocks;*/
			} else {
				struct heap_block *usethis;
				fprintf(stderr, "(before: fp = %p) ", fp);
				
				usethis = fp;
				fp = (void *)&fp[1] + size;
				fprintf(stderr, "(after: fp = %p) ", fp);
				fp->size = usethis->size - size - sizeof(struct heap_block);
				fp->next = usethis->next;
				prev->next = fp;
				
				usethis->size = size;
				usethis->next = NULL;
				
				
#if 0
				fp->size -= size;
				fp = (void *)&fp[1] + fp->size;
				fp->size = size;

				p->size -= size;
				p+=p->size;
				p->size = size;
#endif

			}
			fprintf(stderr, "}");
			return &fp[1]; /* skip header */
		}
	}

	fprintf(stderr, "}");
	return NULL;
}

int main()
{
	char *foo;

	fprintf(stderr, "sizeof(struct heap_block) = %ld\n", sizeof(struct heap_block));
	
	heap_init();
	fprintf(stderr, "heap = %p\n", heap);
	kmalloc(128*1024);

	fprintf(stderr, "\n");
	while ((foo = kmalloc(1024))) {
		printf("%p\n", foo);
	}

	return 0;
}
