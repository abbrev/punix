#ifndef _FS_H_
#define _FS_H_

struct fsops;

struct filesystem {
	struct fsops *fsops;
	ino_t root_inum;
	/* other filesystem stuff */
	/* union of filesystem-specific information */
};

/* filesystem operations */
struct fsops {
	struct inode *(*read_inode)(struct filesystem *fs, ino_t inum);
	void (*write_inode)(struct filesystem *fs, struct inode *inode);
	/* other operations, eg, get info about the fs */
};

#endif
