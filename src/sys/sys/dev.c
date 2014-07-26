#include <string.h>
#include <errno.h>

#include "punix.h"
#include "proc.h"
#include "queue.h"
#include "buf.h"
#include "dev.h"
#include "inode.h"
#include "globals.h"

STARTUP(void nodev())
{
	P.p_error = ENODEV;
}

STARTUP(void nulldev())
{
}

/* FIXME: move these to their appropriate home */
extern void flopen(dev_t, int);
extern void flclose(dev_t, int);
extern void flstrategy(struct buf *);
extern const struct devtab fltab;

const struct bdevsw bdevsw[] = {
	{ NULL, flopen, flclose, flstrategy, &fltab }, /* flash */
	{ NULL, 0, 0, 0, 0 }
};

const int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]) - 1;

extern const struct fileops vt_fileops, misc_fileops, link_fileops, audio_fileops, devtty_fileops;

/* eventually put the following devices into cdevsw:
 * USB port (for HW3)
 */
const struct fileops *cdevsw[] = {
	&misc_fileops,     /* misc */
#if 1
	&vt_fileops,       /* vt */
#endif
	&devtty_fileops,   /* tty */
	&link_fileops,     /* link */
	&audio_fileops,    /* audio */
	NULL,
};

const int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]) - 1;
