#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "inode.h"

/*
 * The Punix FS is similar to other UNIX filesystems in that the inodes contain
 * direct and indirect (single, double, triple) blocks
 * pfs_bmap() converts the logical block number "lbn" for inode "inop" into
 * a device block number
 */

/* return the device block number for the inode and logical block number */
STARTUP(long pfs_bmap(struct inode *inop, long lbn))
{
	/* FIXME */
	return -1;
}

