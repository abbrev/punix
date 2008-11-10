#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

/* $Id$ */

#include <sys/types.h>
#include <time.h>

struct stat {
	dev_t	st_dev;	/* device ID of device containing file */
	ino_t	st_ino;	/* file serial number */
	mode_t	st_mode;	/* mode of file */
	nlink_t	st_nlink;	/* number of hard links to the file */
	uid_t	st_uid;	/* user id of file */
	gid_t	st_gid;	/* group id of file */
	/* [XSI] [x> */
	dev_t	st_rdev;	/* device ID (if file is character or block special */
	/* <x] */
	off_t	st_size;	/* for regular files, the file size in bytes.
				   for symlinks, the length in bytes of the
				   pathname contained in the symlink */
	time_t	st_atime;	/* time of last access */
	time_t	st_mtime;	/* time of last data modification */
	time_t	st_ctime;	/* time of last status change */
	/* [XSI] [x> */
	blksize_t	st_blksize;	/* file system-specific preferred I/O block size for this file */
	blkcnt_t	st_blocks;	/* number of blocks allocated for this file */
	/* <x] */
};

/* [XSI] [x> */
/* Type of file */
/* XXX are these good values for S_IF*? */
#define S_IFMT		0070000
#define S_IFBLK		0010000	/* block special */
#define S_IFCHR		0020000	/* character special */
#define S_IFIFO		0030000	/* fifo special */
#define S_IFREG		0040000	/* regular */
#define S_IFDIR		0050000	/* directory */
#define S_IFLNK		0060000	/* symbolic link */
#define S_IFSOCK	0070000	/* socket */
/* <x] */

/* file modes */

#define S_ISUID		0004000	/* set user ID on execution */
#define S_ISGID		0002000	/* set group ID on execution */
/* [XSI] [x> */
#define S_ISVTX		0001000	/* on directories, restricted deletion flag */
/* <x] */

/* Read, write, execute/search by owner. */
#define S_IRWXU	(S_IRUSR | S_IWUSR | S_IXUSR)
#define S_IRUSR		0000400	/* read permission, owner */
#define S_IWUSR		0000200	/* write permission, owner */
#define S_IXUSR		0000100	/* execute/search permission, owner */

#define S_IRWXG	(S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRGRP		0000040	/* read permission, group */
#define S_IWGRP		0000020	/* write permission, group */
#define S_IXGRP		0000010	/* execute/search permission, group */

#define S_IRWX	(S_IROTH | S_IWOTH | S_IXOTH)
#define S_IROTH		0000004	/* read permission, others */
#define S_IWOTH		0000002	/* write permission, others */
#define S_IXOTH		0000001	/* execute/search permission, others */

#define S_ISBLK(m)	((m)&S_IFMT == S_IFBLK)
#define S_ISCHR(m)	((m)&S_IFMT == S_IFCHR)
#define S_ISDIR(m)	((m)&S_IFMT == S_IFDIR)
#define S_ISFIFO(m)	((m)&S_IFMT == S_IFIFO)
#define S_ISREG(m)	((m)&S_IFMT == S_IFREG)
#define S_ISLNK(m)	((m)&S_IFMT == S_IFLNK)
#define S_ISSOCK(m)	((m)&S_IFMT == S_IFSOCK)

/* we don't support any of the following (at least not yet) */
#define S_TYPEISMQ(buf)		0	/* message queue */
#define S_TYPEISSEM(buf)	0	/* semaphore */
#define S_TYPEISSHM(buf)	0	/* shared memory object */

int	chmod(const char *__path, mode_t __mode);
int	fchmod(int __fd, mode_t __mode);
int	fstat(int __fd, struct stat *__buf);
int	lstat(const char *__path, struct stat *__buf);
int	mkdir(const char *__path, mode_t __mode);
int	mkfifo(const char *__path, mode_t __mode);
/* [XSI] [x> */
int	mknod(const char *__path, mode_t __mode, dev_t __dev);
/* <x] */
int	stat(const char *__path, struct stat *__buf);
mode_t	umask(mode_t __mask);

#endif
