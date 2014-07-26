#include "punix.h"

int kputchar(int c);

STARTUP(int kputs(char *s))
{
	register char *i;
	for (i = s; *i; ++i)
		kputchar(*i);
	
	return 0;
}
