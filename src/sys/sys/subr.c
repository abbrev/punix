#include <errno.h>

#include "punix.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* put the character for the user's read call */
STARTUP(int passc(int ch))
{
	// we should check the buffer before we do a passc()
	if (badbuffer(P.p_base, 1)) {
		P.p_error = EFAULT;
		return -1;
	}
	*P.p_base++ = ch;
	++P.p_offset;
	--P.p_count;
	return (P.p_count == 0 ? -1 : 0);
}

/* Get the next character from the user's write call. Return -1 if p_count == 0 */
STARTUP(int cpass())
{
	int ch;
	
	if (P.p_count == 0)
		return -1;
	// we should check the buffer before we do a cpass()
	if (badbuffer(P.p_base, 1)) {
		P.p_error = EFAULT;
		return -1;
	}
	ch = (unsigned char)*P.p_base++;
	++P.p_offset;
	--P.p_count;
	return ch;
}
