#include <execinfo.h>

#include "punix.h"

int kputs(const char *s);

STARTUP(void panic(const char *s))
{
	void *bt[18];
	int i, n;
	kprintf("\nkernel panic: %s    \n", s);
	n = stacktrace(bt, 18);
	kprintf("stacktrace:    \n", n);
	for (i = 0; i < n; ++i)
		kprintf("%06lx    \n", bt[i]);
	while (1)
		nop();
}

void warn(const char *s, long value)
{
	kprintf("%s: %ld\n", s, value);
}
