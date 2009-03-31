#include "punix.h"

int kputs(const char *s);

STARTUP(void panic(const char *s))
{
	kprintf("\nkernel panic: %s", s);
	while (1)
		nop();
}

void warn(const char *s, long value)
{
	kprintf("%s: %ld\n", s, value);
}
