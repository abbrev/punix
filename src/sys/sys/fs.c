#include <string.h>
#include <errno.h>

#include "punix.h"
#include "globals.h"
#include "fs.h"
#include "inode.h"

#define MAXFSTYPENAMELEN 16

/* filesystem name and function to read the filesystem's superblock */
struct fstype {
	const char name[MAXFSTYPENAMELEN];
	unsigned long flags;
	struct filesystem *(*read_superblock)(dev_t);
};

enum {
	FS_NONE = 0,
	FS_NOAUTO = 1,
};

struct filesystem *ramfs_read_superblock(dev_t);

/* list of filesystem types */
static const struct fstype fstypes[] = {
	{ "ramfs", FS_NOAUTO, ramfs_read_superblock },
	{ NULL, 0, NULL },
};

void sys_mount()
{
	/* arguments are borrowed from Linux */
	struct a {
		const char *source, *target, *name;
		unsigned long flags;
		const void *options;
	} *ap = (struct a *)P.p_arg;

	struct fstype *f;
	struct filesystem *fs = NULL;
	struct inode *root_inode = NULL;
	int isauto;
	dev_t dev = (dev_t)-1; /* TODO: resolve source to a dev_t */

	isauto = (strcmp(ap->name, "auto") == 0);
	if (isauto)
		P.p_error = ENODEV;
	for (f = &fstypes[0]; f->name; ++f) {
		if (isauto && (f->flags & FS_NOAUTO))
			continue;
		if (!isauto && strcmp(ap->name, f->name) != 0)
			continue;
		fs = f->read_superblock(dev);
		if (fs) goto found;
	}
	return;
found:
	root_inode = fs->fsops->read_inode(fs, fs->root_inum);
	/* TODO: mount this filesystem */
}

struct inode *namei(const char *path)
{
	/* TODO: write this */
	return NULL;
}
