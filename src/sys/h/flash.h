#ifndef _FLASH_H_
#define _FLASH_H_

struct flashblock {
	short status;
	long blockno;
	unsigned short data[BLOCKSIZE/2];
} __attribute__((packed));

#endif
