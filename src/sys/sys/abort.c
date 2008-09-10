#include <stdlib.h>

#include "punix.h"

void abort(void)
{
	panic("Aborted");
}
