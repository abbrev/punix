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

short FlashWrite(const void *src asm("%a2"), void *dest asm("%a3"),
                 size_t size asm("%d3"));
short FlashErase(void *dest asm("%a2"));
void flashinit();


#endif
