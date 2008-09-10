#include "punix.h"

int kputs(const char *s);

STARTUP(void panic(const char *s))
{
	kputs("\nkernel panic: ");
	kputs(s);
	while (1)
		/*halt()*/;
}
