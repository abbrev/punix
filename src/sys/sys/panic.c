#include <execinfo.h>

#include "punix.h"

int kputs(const char *s);

STARTUP(void panic(const char *s))
{
#define BACKTRACE_BUF_SIZE 20
	void *bt[BACKTRACE_BUF_SIZE];
	int i, n;
	kprintf("\nkernel panic: %s    \n", s);
	n = backtrace(bt, BACKTRACE_BUF_SIZE);
	kprintf("backtrace:    \n", n);
	for (i = 0; i < n; ++i)
		kprintf("%06lx    \n", bt[i]);
	while (1)
		nop();
}

void warn(const char *s, long value)
{
	kprintf("%s: %ld\n", s, value);
}
