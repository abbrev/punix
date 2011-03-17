#include <stdlib.h>

static void swap(void *a, void *b, size_t size)
{
	char x;
	while (size) {
		x = *(char *)a;
		*((char *)a)++ = *(char *)b;
		*((char *)b)++ = x;
		--size;
	}
}

/* Gnome sort! */
void qsort(void *base, size_t n, size_t size,
           int (*compare)(const void *, const void *))
{
	size_t i = 1;
	void *a, *b;
	while (i < n) {
		a = base + size*i;
		b = base + size*(i-1);
		if (compare(a, b) < 0) {
			swap(a, b, size);
			if (i > 1)
				--i;
		} else {
			++i;
		}
	}
}

