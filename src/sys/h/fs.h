#ifndef _FS_H_
#define _FS_H_

#define MAXFSTYPENAMELEN 16

struct fsops;
struct fstype;

struct filesystem {
	struct fsops *fsops;
	ino_t root_inum;
	/* other filesystem stuff */
	/* union of filesystem-specific information */
};

/* filesystem operations */
struct fsops {
	struct inode *(*alloc_inode)(struct filesystem *);
	void (*free_inode)(struct inode *);

	int (*read_inode)(struct filesystem *fs, struct inode *);
	void (*write_inode)(struct filesystem *fs, struct inode *inode);
	/* other operations, eg, get info about the fs */
};

/* this describes a file system type and is used to mount a file system */
struct fstype {
	const char name[MAXFSTYPENAMELEN];
	unsigned long flags;
	int (*read_filesystem)(struct fstype *,
	                       struct filesystem *, int flags,
			       const char *devname, const void *data);
};

/* fstype.flags */
enum {
	FS_NONE = 0,
	FS_NOAUTO = 1,
};

#endif
