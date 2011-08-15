#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void __assert(const char *expr, const char *file, unsigned line)
{
	fprintf(stderr, "%s:%u: Assertion `%s' failed.\n", file, line, expr);
	abort();
}
