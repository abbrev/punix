/*
 * Punix, Puny Unix kernel
 * Copyright 2005, 2007 Christopher Williams
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	_SYS_FILE_H_
#define	_SYS_FILE_H_

#include <limits.h>

/* FREAD and FWRITE are for file.f_flag member */
#define FREAD  O_RDONLY
#define FWRITE O_WRONLY

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct file {
	int		f_flag;		/* see below */
	short		f_type;		/* descriptor type */
	unsigned short	f_count;	/* reference count */
	struct inode *	f_inode;	/* pointer to inode structure */
	short		f_msgcount;	/* references from message queue */
	off_t		f_offset;
};

extern struct file file[NFILE];

/* values for file.f_type */
#define	DTYPE_INODE	1	/* file */
#define	DTYPE_SOCKET	2	/* not used in Punix */
#define	DTYPE_PIPE	3	/* */

/*
struct fileops {
	int	(*fo_rw)();
	int	(*fo_ioctl)();
	int	(*fo_select)();
	int	(*fo_close)();
};
*/

struct file *getf(int fd);
int ufalloc(int);
int falloc(void);

#define	GETF(fp, fd) \
do { \
	if ((unsigned)(fd) >= NOFILE || ((fp) = P.p_ofile[fd]) == NULL) { \
		P.p_error = EBADF; \
		return; \
	} \
} while (0)

#define EACHFILE(fp) ((fp) = &G.file[0]; (fp) < &G.file[NFILE]; ++(fp))

#endif	/* _SYS_FILE_H_ */
