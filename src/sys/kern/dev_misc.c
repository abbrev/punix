/*
 * dev_misc.c, miscellaneous system devices
 * Copyright 2008 Christopher Williams
 * 
 * $Id: dev_misc.c,v 1.3 2008/04/18 02:20:09 fredfoobar Exp $
 * 
 * This implements /dev/null, /dev/zero, /dev/random, and /dev/full.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* device minor values */
#define DEVNULL   0
#define DEVZERO   1
#define DEVRANDOM 2
#define DEVFULL   3
/* following are not implemented */
#define DEVSTDIN  4
#define DEVSTDOUT 5
#define DEVSTDERR 6

/*
 * a simple linear congruential pseudo-random number generator
 * Note: this generator is not cryptographically secure. Do not use it for
 * encryption!
 */
STARTUP(int rand())
{
	G.prngseed = 1103515245UL*G.prngseed + 12345UL;
	return ((unsigned)(G.prngseed / 65536) % 32768);
}

STARTUP(void srand(unsigned s))
{
	G.prngseed = s;
}

STARTUP(void miscopen(dev_t dev, int rw))
{
	/* this should support DEVTTY, DEVSTDIN, DEVSTDOUT, and DEVSTDERR by
	 * somehow re-routing to the real device. */
}

STARTUP(void miscread(dev_t dev))
{
	int minor = MINOR(dev);
	
	switch (minor) {
	case DEVFULL:
	case DEVZERO:
		memset(P.p_base, 0, P.p_count);
		P.p_count = 0;
		break;
	case DEVRANDOM:
		while (passc(rand() >> 7) >= 0) /* XXX constant */
			;
		break;
	}
}

STARTUP(void miscwrite(dev_t dev))
{
	int minor = MINOR(dev);
	
	switch (minor) {
	case DEVZERO:
	case DEVNULL:
	case DEVRANDOM:
		P.p_count = 0;
		break;
	case DEVFULL:
		P.p_error = ENOSPC;
		break;
	}
}

STARTUP(void miscioctl(dev_t dev, int cmd, void *cmargs))
{
}
