#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "inode.h"

/*
 * Punix FS uses the following scheme for block numbers:
 * Inode number 'n' is in block number n << 16.
 * File contents for inode 'n' are found in blocks n << 16 + 1 through
 * n << 16 + x, where 'x' is the number of file blocks.
 * 
 * In the future, this may be modified so that inode 'n' is in block number
 * (n & ~1) << 15, and the inode's position within the block is determined by
 * its least significant bit (0 or 1, first or second half of block).
 * Likewise, file content blocks reflect this bit in their block numbers.
 */

/* It is up to the block device driver (flash) to retrieve blocks given a
 * block number, whether the block currently exists in flash or not; if it does
 * not exist yet, the flash driver pretends that it does and returns a clear
 * block (all 0's) to the filesystem. This way, the filesystem can easily
 * support sparse files, or files with "holes" in them, simply by not writing
 * to these blocks. Unfortunately, this makes file system accounting more
 * difficult (eg, for the df utility).
 */

/* return the device block number for the inode and logical block number */
STARTUP(long bmap(struct inode *ip, long bn))
{
	return (ip->i_number << 16) + bn + 1; /* XXX does this suffice? */
	
	/* Whew! That was a challenge. */
}

