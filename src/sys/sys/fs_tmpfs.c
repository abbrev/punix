/* fs_tmpfs.c, temporary filesystem driver for Punix */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "punix.h"
#include "globals.h"
#include "fs.h"

struct filesystem *tmpfs_read_filesystem(dev_t);

struct fstype tmpfs_fstype = {
	.name = "tmpfs",
	.flags = FS_NOAUTO,
	.read_filesystem = tmpfs_read_filesystem,
};

struct filesystem *tmpfs_read_filesystem(dev_t dev)
{
	return NULL;
}

int tmpfs_file_open(struct file *fp, struct inode *ip);
ssize_t tmpfs_file_read(struct file *fp, void *buf, size_t count);
ssize_t tmpfs_file_write(struct file *fp, void *buf, size_t count);
off_t generic_file_lseek(struct file *fp, off_t offset, int whence);

struct fileops tmpfs_file_ops = {
	.open = tmpfs_file_open,
	.read = tmpfs_file_read,
	.write = tmpfs_file_write,
	.lseek = generic_file_lseek,
};

struct tmpfs_inode {
	short signature;
	mode_t mode;
	uid_t owner;
	gid_t group;
	unsigned short nlink;
	union {
		size_t size;          /* number of bytes in file */
		dev_t rdev;           /* for character/block special files */
	};
	time_t atime;          /* time last accessed */
	time_t mtime;          /* time last modified */
	time_t ctime;          /* time last changed */

	size_t allocsize;
	void *data;
};

#define TMPFS_NAMELEN 15

struct tmpfs_direntry {
	char name[TMPFS_NAMELEN+1];
	struct tmpfs_inode *inode;
	struct tmpfs_direntry *next;
};

int tmpfs_file_open(struct file *fp, struct inode *ip)
{
	/* do we need to do anything else here?? */
	fp->f_ops = ip->i_fops;
	return 0;
}

ssize_t tmpfs_file_read(struct file *fp, void *buf, size_t count)
{
	struct inode *ip = fp->f_inode;
	struct tmpfs_inode *tmpfs_ip = (struct tmpfs_inode *)ip->i_num;

	if (count > LONG_MAX)
		count = LONG_MAX;

	if (fp->f_offset >= ip->i_size)
		return 0;
	if (fp->f_offset + count > ip->i_size)
		count = ip->i_size - fp->f_offset;
	
	if (copyout(buf, tmpfs_ip->data+fp->f_offset, count)) {
		P.p_error = EFAULT;
		return -1;
	}
	fp->f_offset += count;
	return count;
}

ssize_t tmpfs_file_write(struct file *fp, void *buf, size_t count)
{
	/* TODO: write this (will be similar to read) */
	P.p_error = ENOSPC;
	return -1;
}

#if 0 /* for reference */
struct fileops {
	int (*open)(struct file *, struct inode *);
	ssize_t (*read)(struct file *, void *, size_t);
	ssize_t (*write)(struct file *, void *, size_t);
	off_t (*lseek)(struct file *, off_t, int);
};

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct file {
	struct list_head f_list;
	unsigned char	f_flag;		/* see below */
	unsigned char	f_type;		/* descriptor type */
	unsigned short	f_count;	/* reference count */
	struct inode *	f_inode;	/* pointer to inode structure */
	struct fileops *f_ops;
#if 0
	short		f_msgcount;	/* references from message queue */
#endif
	off_t		f_offset;
};

#endif
