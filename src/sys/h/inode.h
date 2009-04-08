/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)inode.h	1.4 (2.11BSD GTE) 1995/12/24
 */

#ifndef _INODE_H_
#define _INODE_H_

/* flags for namei() */
enum { INO_SOUGHT, INO_CREATE, INO_DELETE };

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 */

struct inode {
	struct inode *i_next, *i_prev;
	
	unsigned short	i_flag;
	unsigned short	i_count;	/* reference count */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	struct	fs *i_fs;	/* file sys associated with this inode */
	
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
	time_t	i_ctime;		/* time created */
};

#if 0
/*
 * Invalidate an inode. Used by the namei cache to detect stale
 * information. In order to save space and also reduce somewhat the
 * overhead - the i_id field is made into a u_short.  If a pdp-11 can 
 * invalidate 100 inodes per second, the cache will have to be invalidated 
 * in about 11 minutes.  Ha!
 * Assumes the cacheinvalall routine will map the namei cache.
 */
#define cacheinvalall _cinvall

#define cacheinval(ip) \
	(ip)->i_id = ++nextinodeid; \
	if (nextinodeid == 0) \
		cacheinvalall();

unsigned short	nextinodeid;		/* unique id generator */
#endif

extern struct inode inode[];		/* the inode table itself */
#if 0
struct inode *inodeNINODE;	/* the end of the inode table */
int	ninode;			/* the number of slots in the table */
#endif

extern struct	inode *rootdir;		/* pointer to inode of root directory */

struct	inode *iget(dev_t dev, struct fs *, ino_t ino);
void iput(struct inode *);

struct	inode *getinode();
struct	inode *ialloc(dev_t dev);
struct	inode *maknode();
struct	inode *namei();

/* i_flag */
#define	ILOCKED		0x0001		/* inode is locked */
#define	IUPD		0x0002		/* file has been modified */
#define	IACC		0x0004		/* inode access time to be updated */
#define	IMOUNT		0x0008		/* inode is mounted on */
#define	IWANT		0x0010		/* some process waiting on lock */
#define	ITEXT		0x0020		/* inode is pure text prototype */
#define	ICHG		0x0040		/* inode has been changed */
#define	ISHLOCK		0x0080		/* file has shared lock */
#define	IEXLOCK		0x0100		/* file has exclusive lock */
#define	ILWAIT		0x0200		/* someone waiting on file lock */
#define	IMOD		0x0400		/* inode has been modified */
#define	IRENAME		0x0800		/* inode is being renamed */
#define	IPIPE		0x1000		/* inode is a pipe */
#define	IRCOLL		0x2000		/* read select collision on pipe */
#define	IWCOLL		0x4000		/* write select collision on pipe */
#define	IXMOD		0x8000		/* inode is text, but impure (XXX) */

/* i_mode */
#define	IFMT		0170000		/* type of file */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */
#define	ISUID		0004000		/* set user id on execution */
#define	ISGID		0002000		/* set group id on execution */
#define	ISVTX		0001000		/* save swapped text even after use */
#define	IREAD		0000400		/* read, write, execute permissions */
#define	IWRITE		0000200
#define	IEXEC		0000100

#define EACHINODE(ip) (ip = &G.inode[0]; ip < &G.inode[NINODE]; ++ip)

#if 0 /* these may be usable sometime. */
#define	ILOCK(ip) { \
	while ((ip)->i_flag & ILOCKED) { \
		(ip)->i_flag |= IWANT; \
		slp((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_flag |= ILOCKED; \
}

#define	IUNLOCK(ip) { \
	(ip)->i_flag &= ~ILOCKED; \
	if ((ip)->i_flag&IWANT) { \
		(ip)->i_flag &= ~IWANT; \
		wakeup((caddr_t)(ip)); \
	} \
}

#define	IUPDAT(ip, t1, t2, waitfor) { \
	if (ip->i_flag&(IUPD|IACC|ICHG|IMOD)) \
		iupdat(ip, t1, t2, waitfor); \
}

#define	ITIMES(ip, t1, t2) { \
	if ((ip)->i_flag&(IUPD|IACC|ICHG)) { \
		(ip)->i_flag |= IMOD; \
		if ((ip)->i_flag&IACC) \
			(ip)->i_atime = (t1)->tv_sec; \
		if ((ip)->i_flag&IUPD) \
			(ip)->i_mtime = (t2)->tv_sec; \
		if ((ip)->i_flag&ICHG) \
			(ip)->i_ctime = time.tv_sec; \
		(ip)->i_flag &= ~(IACC|IUPD|ICHG); \
	} \
}
#endif /* 0 */

#endif
