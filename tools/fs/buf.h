#ifndef _BUF_H_
#define _BUF_H_

#define BLOCK_SIZE 128

struct buf {
	//struct list_head b_list;
	long b_blkno;
	char *b_addr;
};

struct buf *getblk(long blkno);
struct buf *bread(long blkno);
void bwrite(struct buf *bp);
void brelse(struct buf *bp);

extern char *blockdev;
extern unsigned long blocks;

#endif
