#ifndef _PFS_H_
#define _PFS_H_

struct pfs_sb_info {
	unsigned short s_version;
	char s_label[16];
	unsigned s_block_count;
	unsigned s_inode_count;
	unsigned s_rsvd_block_count;
	unsigned s_first_data_block;
};

/*
size  name                     value
4     magic value              "PFS!" (or something like that)
2     version                  0
2     state                    
16    volume name              zero-padded string
2     block count              size of block device (in blocks)
2     inode count              variable during usage
2     reserved block count     tunable
2     first data block
2     log(block size)          7
4     mount time
4     write time
2     mount count
2     maximal mount count
4     last check time
4     max time between checks
*/

struct pfs_inode_info {
	unsigned short i_block_depth;
	unsigned short i_blocks[6];
};

struct inode;
struct super_block;

void pfs_write_super(struct super_block *);
void pfs_write_inode(struct inode *);

#endif
