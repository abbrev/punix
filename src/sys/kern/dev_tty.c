/* dev_tty.c, /dev/tty character device file */

#include <errno.h>

#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* FIXME! this probably needs to make a new inode for the tty device. even if all open files are closed, we still need to be able to open the controlling tty */
STARTUP(void devttyopen(struct file *fp, int rw))
{
	if (!P.p_ttyp) {
		P.p_error = ENXIO;
		return;
	}
}

STARTUP(void devttyclose(struct file *fp, int rw))
{
}

/* FIXME: we also have to be able to read from/write to the (previous) controlling tty even when the process detaches from its controlling tty */
STARTUP(void devttyread(dev_t dev))
{
	cdevsw[MAJOR(dev)].d_read(dev);
}

STARTUP(void devttywrite(dev_t dev))
{
	cdevsw[MAJOR(dev)].d_write(dev);
}

STARTUP(void devttyioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
	/* FIXME: handle the TIOCNOTTY ioctl command */
	cdevsw[MAJOR(dev)].d_ioctl(dev, cmd, cmarg, flag);
}
