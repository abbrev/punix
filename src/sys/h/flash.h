#ifndef _FLASH_H_
#define _FLASH_H_

struct flashblock {
	long blockno;
	char data[BLOCKSIZE];
};

struct flash_cache_entry {
	long blkno;
	struct flashblock *fbp;
};

#endif
