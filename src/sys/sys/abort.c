#include <stdlib.h>

#include "punix.h"

STARTUP(void abort(void))
{
	panic("Aborted");
}
