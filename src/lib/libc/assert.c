#include <assert.h>
#include <stdlib.h>

#include "punix.h"

STARTUP(void __assert(const char *expr, const char *file, unsigned line))
{
	kprintf("%s:%u: Assertion `%s' failed.\n", file, line, expr);
	abort();
}
