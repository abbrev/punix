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

STARTUP(void devttyopen(dev_t dev, int rw))
{
	if (!P.p_ttyp)
		P.p_error = ENXIO;
}

STARTUP(void devttyclose(dev_t dev, int rw))
{
}

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
	cdevsw[MAJOR(dev)].d_ioctl(dev, cmd, cmarg, flag);
}
