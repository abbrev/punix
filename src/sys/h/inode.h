#ifndef _INODE_H_
#define _INODE_H_

struct inodeops;

struct inode {
	struct inodeops *iops;
	int refcount;
	
	//unsigned short	flag;
	dev_t	dev;
	ino_t	num;
	struct	fs *fs;
	
	/* following fields are stored on the device */
	unsigned short	mode;	/* mode and type of file */
	unsigned short	nlink;	/* number of links to file */
	uid_t	uid;		/* owner's user id */
	gid_t	gid;		/* owner's group id */
	union {
		off_t	size;		/* number of bytes in file */
		dev_t	rdev;		/* for character/block special files */
	};
	time_t	atime;		/* time last accessed */
	time_t	mtime;		/* time last modified */
	time_t	ctime;		/* time last changed */

	/* union of filesystem-specific information here */
};

struct inodeops {
	ssize_t (*read)(struct inode *, off_t pos, void *buf, size_t count);
	ssize_t (*write)(struct inode *, off_t pos, void *buf, size_t count);
	struct inode *(*lookup)(struct inode *dirinode, const char *name, int follow);
	/* other operations */
};

#endif


