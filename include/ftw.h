#ifndef _FTW_H_
#define _FTW_H_

/* mostly POSIX compliant? */

#include <sys/stat.h>

struct FTW {
	int	base;
	int	level;
};

#define FTW_F	0
#define FTW_D	1
#define FTW_DNR	2
#define FTW_DP	3
#define FTW_NS	4
#define FTW_SL	5
#define FTW_SLN	6

#define FTW_PHYS	0x1
#define FTW_MOUNT	0x2
#define FTW_DEPTH	0x4
#define FTW_CHDIR	0x8

int	ftw(const char *__dir, int (*__fn)(const char *__file,
		const struct stat *__sb, int __flag), int __depth);
int	nftw(const char *__dir, int (*__fn)(const char *__file,
		const struct stat *__sb, int __flag, struct FTW *__s),
		int __depth, int __flags);

#endif
