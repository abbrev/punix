#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buf.h"
#include "pfs.h"
#include "fs.h"

static char *put_short(char *cp, short n)
{
	if (!cp) return NULL;
	*cp++ = n >> 8;
	*cp++ = n;
	return cp;
}

static char *put_long(char *cp, long n)
{
	cp = put_short(cp, n >> 16);
	return put_short(cp, n);
}

static char *put_string(char *cp, char *s, size_t n)
{
	strncpy(cp, s, n);
	return cp + n;
}

/*
 * NB: the on-device layout for the super block and
 * inode structures are not completely finalized yet
 */
void pfs_write_super(struct super_block *sb)
{
	char *cp;
	struct buf *bp;
	
	bp = getblk(0);
	if (!bp) return;
	cp = bp->b_addr;
	cp = put_string(cp, "PFS0", 4);
	cp = put_short(cp, sb->u.pfs_sb.s_version);
	cp = put_string(cp, sb->u.pfs_sb.s_label, 16);
	cp = put_short(cp, sb->u.pfs_sb.s_block_count);
	cp = put_short(cp, sb->u.pfs_sb.s_inode_count);
	cp = put_short(cp, sb->u.pfs_sb.s_rsvd_block_count);
	cp = put_short(cp, sb->u.pfs_sb.s_first_data_block);
	memset(cp, 0, bp->b_addr + 128 - cp);
	bwrite(bp);
#if 0
struct pfs_sb_info {
	unsigned short s_version;
	char s_label[16];
	unsigned s_block_count;
	unsigned s_inode_count;
	unsigned s_rsvd_block_count;
	unsigned s_first_data_block;
};

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
#endif
}

void pfs_write_inode(struct inode *ip)
{
	int i;
	char *cp;
	struct buf *bp;
	long blkno;
	long blocks = ip->i_sb->u.pfs_sb.s_block_count;
	ino_t ino = ip->i_ino;
	
	blkno = blocks - (ino * 32 / 128) - 1; /* XXX */
	
	bp = getblk(blkno);
	if (!bp) return;
	
	cp = bp->b_addr + 32 * (ino%4);
	
	cp = put_short(cp, ip->i_mode);
	cp = put_short(cp, ip->i_nlink);
	cp = put_short(cp, ip->i_uid);
	cp = put_short(cp, ip->i_gid);
	cp = put_long(cp, ip->i_mtime);
	cp = put_long(cp, ip->i_ctime);
	cp = put_long(cp, ip->i_size | (ip->u.pfs_i.i_block_depth << 24));
	for (i = 0; i < 6; ++i)
		cp = put_short(cp, ip->u.pfs_i.i_blocks[i]);
	
	bwrite(bp);
#if 0
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
#endif
#if 0
	mode_t mode;
	unsigned short nlinks;
	uid_t owner, group;
	unsigned long mtime, ctime;
	unsigned long size;
	int bindex_depth;
	unsigned short blocks[6];
#endif
}
