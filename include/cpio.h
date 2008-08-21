#ifndef _CPIO_H_
#define _CPIO_H_

/* $Id: cpio.h,v 1.1 2007/05/03 08:59:57 fredfoobar Exp $ */

/* extracted almost directly from POSIX (so it should be compliant) */

/*
field name	length		notes
c_magic		6		must be "070707"
c_dev		6
c_ino		6
c_mode		6
c_uid		6
c_gid		6
c_nlink		6
c_rdev		6		valid only for chr and blk special files
c_mtime		11
c_namesize	6		count includes terminating NUL in pathname
c_filesize	11		must be 0 for FIFOs and directories
c_name		c_namesize
c_filedata	c_filesize
*/

/* value for c_magic */
#define MAGIC	"070707"

/* values for c_mode */
#define C_IRUSR		0000400 /* Read by owner. */
#define C_IWUSR		0000200 /* Write by owner. */
#define C_IXUSR		0000100 /* Execute by owner. */
#define C_IRGRP		0000040 /* Read by group. */
#define C_IWGRP		0000020 /* Write by group. */
#define C_IXGRP		0000010 /* Execute by group. */
#define C_IROTH		0000004 /* Read by others. */
#define C_IWOTH		0000002 /* Write by others. */
#define C_IXOTH		0000001 /* Execute by others. */
#define C_ISUID		0004000 /* Set user ID. */
#define C_ISGID		0002000 /* Set group ID. */
#define C_ISVTX		0001000 /* On directories, restricted deletion flag. */
#define C_ISDIR		0040000 /* Directory. */
#define C_ISFIFO	0010000 /* FIFO. */
#define C_ISREG		0100000 /* Regular file. */
#define C_ISBLK		0060000 /* Block special. */
#define C_ISCHR		0020000 /* Character special. */
#define C_ISCTG		0110000 /* Reserved. */
#define C_ISLNK		0120000 /* Symbolic link. */
#define C_ISSOCK	0140000 /* Socket. */

#endif
