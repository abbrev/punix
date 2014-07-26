#ifndef _SYS_UTSNAME_H_
#define _SYS_UTSNAME_H_

struct utsname {
	char sysname[9];
	char nodename[9];
	char release[9];
	char version[33];
	char machine[9];
};

int uname(struct utsname *);

#endif
