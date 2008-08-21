#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

/* $Id: types.h,v 1.2 2008/01/11 13:35:45 fredfoobar Exp $ */

/* this should be POSIX-compliant */

typedef signed int	blkcnt_t;
typedef signed int	blksize_t;
typedef long		clock_t;
typedef unsigned short	dev_t;
typedef unsigned long	fsblkcnt_t;
typedef unsigned long	fsfilcnt_t;
typedef unsigned int	gid_t;
typedef int		id_t;
typedef unsigned long	ino_t;
typedef unsigned long	mode_t;
typedef unsigned int	nlink_t;
typedef signed long	off_t;
typedef signed int	pid_t;
typedef unsigned long	size_t;
typedef signed long	ssize_t;
typedef signed long	suseconds_t;
typedef long		time_t;
typedef unsigned int	uid_t;
typedef unsigned long	useconds_t;

#endif /* _SYS_TYPES_H_ */
