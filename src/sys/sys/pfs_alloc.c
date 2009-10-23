/* clean pfs_alloc.c */

#include <string.h>

#include "punix.h"
#include "proc.h"
#include "buf.h"
#include "dev.h"
#include "fs.h"
#include "inode.h"
#include "mount.h"
#include "queue.h"
#include "globals.h"


ino_t pfs_inode_alloc(dev_t);
void  pfs_inode_free(dev_t, ino_t);
long pfs_blk_alloc(dev_t);
void pfs_blk_free(dev_t, long);

// allocate an inode number on disk and return it
ino_t pfs_inode_alloc(dev_t dev)
{
	/* TODO: write this */
	return NULLINO;
}

// free the inode number on disk (assumes there are no more references in core or on disk)
void pfs_inode_free(dev_t dev, ino_t ino)
{
	/* TODO: write this */
}

// allocate and return a block number on disk
long pfs_blk_alloc(dev_t dev)
{
	/* TODO: write this */
	return 0;
}

void pfs_inode_trunc(dev_t dev, ino_t ino)
{
	/* TODO: write this */
}

// free the block on disk  
void pfs_blk_free(dev_t dev, long blkno)
{
	/* TODO: write this */
}

#if 0 /* is this even used? */
struct fs *getfs(dev_t dev)
{
	/* TODO: write this */
}
#endif

/* internal "sync" */
void update(void)
{
	/* TODO: write this */
}

