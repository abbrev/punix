#ifndef _DIR_H_
#define _DIR_H_

#ifndef MAXNAMLEN
#define MAXNAMLEN 63
#endif

#define DIRBLKSIZ 128

struct direct {
	ino_t d_ino;
	unsigned char d_reclen; /* specially-encoded record length */
	unsigned char d_namlen;
	char d_name[MAXNAMLEN+1];
};

#endif
