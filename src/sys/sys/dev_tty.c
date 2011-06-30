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

/*
 * NB: we only need an open file handler because we switch our file's inode
 * to the inode of the process's controlling tty
 */
int devtty_open(struct file *fp, struct inode *ip)
{
	if (!P.p_tty) {
		P.p_error = ENXIO;
		return -1;
	}
	i_unref(ip);
	fp->f_inode = ip = P.p_tty;
	i_ref(ip);
	fp->f_ops = ip->i_fops;
	return fp->f_ops->open(fp, ip);
}

const struct fileops devtty_fileops = {
	.open = devtty_open,
};
