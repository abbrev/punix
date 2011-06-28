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
	int isauto;
	dev_t dev = (dev_t)-1; /* TODO: resolve source to a dev_t */

	fs = memalloc(&fssize, 0);
	if (!fs)
		return;
	
	isauto = (strcmp(ap->name, "auto") == 0);
	if (isauto)
		P.p_error = ENODEV;
	for (f = &fstypes[0]; (*f)->name; ++f) {
		if (isauto && ((*f)->flags & FS_NOAUTO))
			continue;
		if (!isauto && strcmp(ap->name, (*f)->name) != 0)
			continue;
		if ((*f)->read_filesystem(*f, fs, ap->flags, ap->source,
		                          ap->options))
			goto found;
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
