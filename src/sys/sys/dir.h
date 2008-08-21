#ifndef _DIR_H_
#define _DIR_H_

struct direct {
	short d_ino;
	char d_name[NAME_MAX];
};

#endif
