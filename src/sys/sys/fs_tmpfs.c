/* fs_tmpfs.c, temporary filesystem driver for Punix */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "punix.h"
#include "globals.h"
#include "fs.h"


#define TMPFS_SIGNATURE 0x1234

int tmpfs_read_filesystem(struct fstype *, struct filesystem *, int flags, const char *devname, const void *data);


struct fstype tmpfs_fstype = {
	.name = "tmpfs",
	.flags = FS_NOAUTO,
	.read_filesystem = tmpfs_read_filesystem,
};

int tmpfs_read_filesystem(struct fstype *fst, struct filesystem *fs,
                          int flags, const char *devname, const void *data)
{
	return 1;
}

int tmpfs_file_open(struct file *fp, struct inode *ip);
ssize_t tmpfs_file_read(struct file *fp, void *buf, size_t count, off_t *);
ssize_t tmpfs_file_write(struct file *fp, void *buf, size_t count, off_t *);
off_t generic_file_lseek(struct file *fp, off_t offset, int whence);

struct fileops tmpfs_file_ops = {
	.open = tmpfs_file_open,
	.read = tmpfs_file_read,
	.write = tmpfs_file_write,
	.lseek = generic_file_lseek,
};

struct inode *tmpfs_alloc_inode(struct filesystem *);
void tmpfs_free_inode(struct inode *);
int tmpfs_read_inode(struct inode *);
void tmpfs_write_inode(struct inode *inode);

struct fsops tmpfs_fsops = {
	.alloc_inode = tmpfs_alloc_inode,
	.free_inode = tmpfs_free_inode,
	//.read_inode = tmpfs_read_inode,
	.write_inode = tmpfs_write_inode,
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

ssize_t tmpfs_file_read(struct file *fp, void *buf, size_t count, off_t *pos)
{
	struct inode *ip = fp->f_inode;
	struct tmpfs_inode *tmpfs_ip = (struct tmpfs_inode *)ip->i_num;

	if (count > LONG_MAX)
		count = LONG_MAX;

	if (*pos >= ip->i_size)
		return 0;
	if (*pos + count > ip->i_size)
		count = ip->i_size - *pos;
	
	if (copyout(buf, tmpfs_ip->data + *pos, count)) {
		P.p_error = EFAULT;
		return -1;
	}
	*pos += count;
	return count;
}

ssize_t tmpfs_file_write(struct file *fp, void *buf, size_t count, off_t *pos)
{
	/* TODO: write this (will be similar to read) */
	P.p_error = ENOSPC;
	return -1;
}


struct inode *tmpfs_alloc_inode(struct filesystem *fs)
{
	struct tmpfs_inode *tfsip = NULL;
	struct inode *ip = NULL;
	size_t size = sizeof(struct tmpfs_inode);

	tfsip = memalloc(&size, 0);
	if (!tfsip) return NULL;
	size = sizeof(struct inode);
	ip = memalloc(&size, 0); // XXX we should call a global inode allocator here
	if (!ip) goto free;

	tfsip->signature = TMPFS_SIGNATURE;
	tfsip->data = NULL;

	ip->i_num = (ino_t)tfsip;
	ip->i_size = 0;
	ip->i_flag |= IACC|IMOD|ICHG;
	ip->i_fs = fs;
	ip->i_count = 0;
	ip->i_nlink = 0;
	i_ref(ip);
	i_lock(ip);

	return ip;

free:
	memfree(tfsip, 0);
	memfree(ip, 0);
	return NULL;
}

void tmpfs_free_inode(struct inode *ip)
{
	// this assumes this inode is unused (no links, file is truncated, etc)
	memfree((void *)ip->i_num, 0);
	memfree(ip, 0); // this should call the system's inode deallocator
}

int tmpfs_read_inode(struct inode *ip)
{
	P.p_error = EINVAL; // XXX
	return -1;
}

void tmpfs_write_inode(struct inode *ip)
{
	struct tmpfs_inode *tfsip = (struct tmpfs_inode *)ip->i_num;
	tfsip->mode = ip->i_mode;
	tfsip->nlink = ip->i_nlink;
	tfsip->owner = ip->i_uid;
	tfsip->group = ip->i_gid;
	tfsip->size = ip->i_size;
	tfsip->atime = ip->i_atime;
	tfsip->mtime = ip->i_mtime;
	tfsip->ctime = ip->i_ctime;
}

#if 0 /* for reference */
struct inode {
	unsigned short i_flag;
	struct inodeops *i_ops;
	struct fileops *i_fops;
	int i_count;
	
	//unsigned short	flag;
	//dev_t	i_dev; // is this needed?
	ino_t	i_num;
	struct	fs *i_fs;
	
	/* following fields are stored on the device */
	unsigned short	i_mode;	/* mode and type of file */
	unsigned short	i_nlink;	/* number of links to file */
	uid_t	i_uid;		/* owner's user id */
	gid_t	i_gid;		/* owner's group id */
	union {
		off_t	i_size;		/* number of bytes in file */
		dev_t	i_rdev;		/* for character/block special files */
	};
	time_t	i_atime;		/* time last accessed */
	time_t	i_mtime;		/* time last modified */
	time_t	i_ctime;		/* time last changed */

	/* union of filesystem-specific information here */
};

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
