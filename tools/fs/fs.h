#ifndef _FS_H_
#define _FS_H_

struct super_block {
	dev_t s_dev;
	unsigned long s_blocksize;
	unsigned char s_rdonly;
	unsigned char s_dirt;
	struct super_operations *s_op;
	unsigned long s_flags;
	unsigned long s_magic;
	unsigned long s_time;
	struct inode *s_covered;
	struct inode *s_mounted;
	/* ... */
	union {
		struct pfs_sb_info pfs_sb;
	} u;
};

struct inode {
	//struct list_head i_list;
	
	unsigned short i_flag;
	unsigned short i_count; /* reference count */
	dev_t i_dev;            /* device where inode resides */
	ino_t i_ino;    /* i number, 1-to-1 with device address */
	unsigned long i_blksize;
	unsigned long i_blocks;
	//struct fs *i_fs;      /* file sys associated with this inode */
	struct super_block *i_sb;
	
	unsigned short i_mode;  /* mode and type of file */
	unsigned short i_nlink; /* number of links to file */
	uid_t i_uid;            /* owner's user id */
	gid_t i_gid;            /* owner's group id */
	union {
		off_t i_size;           /* number of bytes in file */
		dev_t i_rdev;           /* for character/block special files */
	};
	time_t i_atime;         /* time last accessed */
	time_t i_mtime;         /* time last modified */
	time_t i_ctime;         /* time last changed */
	
	union {
		struct pfs_inode_info pfs_i;
	} u;
};

#endif
