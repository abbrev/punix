#include <string.h>
#include <errno.h>

#include "punix.h"
#include "globals.h"
#include "fs.h"
#include "inode.h"

extern const struct fstype tmpfs_fstype;

/* list of filesystem types */
static const struct fstype *fstypes[] = {
	&tmpfs_fstype,
	NULL,
};

struct inode *namei(const char *path)
{
	/* TODO: write this */
	P.p_error = ENOENT;
	return NULL;
}

void sys_mount()
{
	/* arguments are borrowed from Linux */
	struct a {
		const char *source, *target, *name;
		unsigned long flags;
		const void *options;
	} *ap = (struct a *)P.p_arg;

	const struct fstype **f;
	struct filesystem *fs = NULL;
	size_t fssize = sizeof(struct filesystem);
	struct inode *root_inode = NULL;
	struct inode *target_inode = NULL;
	int isauto;

	fs = memalloc(&fssize, 0);
	if (!fs)
		return;
	
	target_inode = namei(ap->target);
	if (!target_inode || !S_ISDIR(target_inode->i_mode)) {
		goto fail;
	}

	isauto = (strcmp(ap->name, "auto") == 0);
	if (isauto)
		P.p_error = ENODEV;
	for (f = &fstypes[0]; (*f)->name; ++f) {
		if (isauto && ((*f)->flags & FS_NOAUTO))
			continue;
		if (!isauto && strcmp(ap->name, (*f)->name) != 0)
			continue;
		if (!(*f)->read_filesystem(*f, fs, ap->flags, ap->source,
		                           ap->options))
			goto found;
	}
	/* couldn't find any matching file system type */
fail:
	memfree(fs, 0);
	iput(target_inode);
	return;

found:
	root_inode = iget(fs, fs->root_inum);
	if (!root_inode)
		return; /* problem getting the inode from device */
	target_inode->i_mount = root_inode;
	iunlock(root_inode);
	iunlock(target_inode);
}
