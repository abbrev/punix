#ifndef _INODE_H_
#define _INODE_H_

struct inodeops;
struct filesystem;

struct inode {
	unsigned short i_flag;
	const struct inodeops *i_ops;
	const struct fileops *i_fops;
	int i_count;
	
	//unsigned short	flag;
	//dev_t	i_dev; // is this needed?
	ino_t	i_num;
	struct	filesystem *i_fs;
	
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

struct inodeops {
	struct inode *(*lookup)(struct inode *dirinode, const char *path, int follow);
	/* other operations */
};

/* i_flag */
#define ILOCKED		0x0001		/* inode is locked */
#define IUPD		0x0002		/* file has been modified */
#define IACC		0x0004		/* inode access time to be updated */
//#define IMOUNT		0x0008		/* inode is mounted on */
#define IWANTED		0x0010		/* some process waiting on lock */
//#define ITEXT		0x0020		/* inode is pure text prototype */
#define ICHG		0x0040		/* inode has been changed */
//#define ISHLOCK		0x0080		/* file has shared lock */
//#define IEXLOCK		0x0100		/* file has exclusive lock */
//#define ILWAIT		0x0200		/* someone waiting on file lock */
#define IMOD		0x0400		/* inode has been modified */
//#define IRENAME		0x0800		/* inode is being renamed */
//#define IPIPE		0x1000		/* inode is a pipe */
//#define IRCOLL		0x2000		/* read select collision on pipe */
//#define IWCOLL		0x4000		/* write select collision on pipe */
//#define IXMOD		0x8000		/* inode is text, but impure (XXX) */

struct inode *iget(struct filesystem *, ino_t);
void iput(struct inode *);

void ilock(struct inode *);
void iunlock(struct inode *);

#endif
