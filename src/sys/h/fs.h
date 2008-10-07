#ifndef _FS_H_
#define _FS_H_

/* from v7, modified to same names as in 2.11BSD */
/*
 * Structure of the super-block
 */
struct  fs {
	unsigned short fs_isize; /* size in blocks of i-list */
	size_t	fs_fsize;        /* size in blocks of entire volume */
	short	fs_nfree;        /* number of addresses in fs_free */
	long	fs_free[NICFREE];/* free block list */
	short	fs_ninode;       /* number of i-nodes in fs_inode */
	ino_t	fs_inode[NICINOD];/* free i-node list */
	char	fs_flock;        /* lock during free list manipulation */
	char	fs_ilock;        /* lock during i-list manipulation */
	char	fs_fmod;         /* super block modified flag */
	char	fs_ronly;        /* mounted read-only flag */
	time_t	fs_time;         /* last super block update */
	/* remainder not maintained by this version of the system */
	size_t	fs_tfree;        /* total free blocks*/
	ino_t	fs_tinode;       /* total free inodes */
	short	fs_m;            /* interleave factor */
	short	fs_n;            /* " " */
	char	fs_fname[6];     /* file system name */
	char	fs_fpack[6];     /* file system pack name */
};

#define blkoff(loc) ((loc) & DEV_BMASK)
#define lblkno(loc) ((loc) >> DEV_BSHIFT)

#endif
