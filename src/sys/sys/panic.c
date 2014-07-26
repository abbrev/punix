#include <string.h>
#include <execinfo.h>

#include "punix.h"
#include "globals.h"

int kputs(const char *s);

#define ON_KEY_PORT (*(volatile char *)0x60001a)
#define ON_KEY_MASK 2

STARTUP(void panic(const char *s))
{
#define BACKTRACE_BUF_SIZE 20
	void *bt[BACKTRACE_BUF_SIZE];
	int i, n;
	spl7();
	strcpy(G.exec_ram, s); // make it visible in debugger
	kprintf("\nkernel panic: %s    \n", s);
	n = backtrace(bt, BACKTRACE_BUF_SIZE);
	kprintf("backtrace:    \n", n);
	for (i = 0; i < n; ++i)
		kprintf("%06lx    \n", bt[i]);
	kprintf("Press [ON] to reboot");
	while (ON_KEY_PORT & ON_KEY_MASK)
		nop();
	while (!(ON_KEY_PORT & ON_KEY_MASK))
		nop();
	boot_start();
}

void warn(const char *s, long value)
{
	kprintf("%s: %ld\n", s, value);
}
