/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef _NAMEI_
#define	_NAMEI_

#ifdef KERNEL
#include "uio.h"
#else
#include <sys/uio.h>
#endif

/*
 * Encapsulation of namei parameters.
 * One of these is located in the u. area to
 * minimize space allocated on the kernel stack.
 */
struct nameidata {
	char *	ni_dirp;		/* pathname pointer */
	short	ni_nameiop;		/* see below */
	short	ni_error;		/* error return if any */
	off_t	ni_endoff;		/* end of useful stuff in directory */
	struct	inode *ni_pdir;		/* inode of parent directory of dirp */
	struct	inode *ni_ip;		/* inode of dirp */
	//enum	uio_seg	ni_segflg;	/* segment flag */
	off_t	ni_offset;		/* offset in directory */
	off_t	ni_count;		/* offset of open slot (off_t?) */
	struct	direct ni_dent;		/* current directory entry */
};

/*
 * namei operations and modifiers
 */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	LOCKPARENT	0x10	/* see the top of namei */
#define NOCACHE		0x20	/* name must not be left in cache */
#define FOLLOW		0x40	/* follow symbolic links */
#define	NOFOLLOW	0x0	/* don't follow symbolic links (pseudo) */

#define	NDINIT(ndp,op,flags,segflg,namep) {\
	(ndp)->ni_nameiop = op | flags; \
	(ndp)->ni_segflg = segflg; \
	(ndp)->ni_dirp = namep; \
	}

#endif
