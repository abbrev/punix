#ifndef _FLASH_H_
#define _FLASH_H_

struct flashblock {
	short status;
	long blockno;
	char data[BLOCKSIZE];
} __attribute__((packed));

#endif
