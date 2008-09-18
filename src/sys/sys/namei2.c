/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions fro redistribution.
 */

#include <errno.h>
#include <string.h>

#include "param.h"
#include "punix.h"
#include "buf.h"
#include "namei.h"
#include "fs.h"
#include "inode.h"
#include "mount.h"
#include "globals.h"

static const volatile int dirchk = 0;

STARTUP(static void dirbad(struct inode *ip, off_t offset, char *how))
{
	kprintf("bad dir I=%u off %ld: %s\n",
	        ip->i_number, offset, how);
}

/* FIXME */
STARTUP(static int dirbadentry(struct direct *ep, int entryoffsetinblock))
{
	return 0;
}

/* FIXME: Punix has a different directory layout, so modify this
 * routine as appropriate
 */

/*
 * Convert a pathname into a pointer to a locked inode.
 * This is a very central and rather complicated routine.
 * If the file system is not maintained in a strict tree hierarchy,
 * this can result in a deadlock situation.
 *
 * The flag argument is LOOKUP, CREATE, or DELETE depending on whether
 * the name is to be looked up, created, or deleted. When CREATE or
 * DELETE is specified, information usable in creating or deleting a
 * directory entry is also calculated. If flag has LOCKPARENT or'ed
 * into it and the target of the pathname exists, namei returns both
 * the target and its parent directory locked. When creating and
 * LOCKPARENT is specified, the target may not be ".". When deleting
 * and LOCKPARENT isspecified, the target may be ".", but the caller
 * must check to insure it does an irele and iput instead of two iputs.
 *
 * The FOLLOW flag is set when symbolic links are to be followed
 * when they occur at the end of the name translation process.
 * Symbolic links are always followed for all other pathname
 * components other than the last.
 *
 * Overall outline of namei:
 *
 * 	copy in name
 * 	get starting directory
 * dirloop:
 * 	check accessibility of directory
 * dirloop2:
 * 	copy next component of name to ndp->ni_dent
 * 	handle degenerate case where name is null string
 * 	search for name in directory, tofound or notfound
 * notfound:
 * 	if creating, return locked directory, leaving info on available slots
 * 	else return error
 * found:
 * 	if at end of path and deleting, return information to allow delete
 * 	if at end of path and rewriting (CREATE and LOCKPARENT), lock target
 * 	  inode and return info to allow rewrite
 * 	if .. and on mounted filesys, look in mount table for parent
 * haveino:
 * 	if symbolic link, massage name in buffer and continue at dirloop
 * 	if more components of name, do next level at dirloop
 * 	return the answer as locked inode
 *
 * NOTE: (LOOKUP | LOCKPARENT) currently returns the parent inode,
 *       but unlocked.
 */
#define mapin(x) 0
#define mapout(x) 0

STARTUP(struct inode *namei(struct nameidata *ndp))
{
	char *cp;
	struct inode *dp = NULL;
	struct fs *fs;
	struct buf *bp = NULL;
	struct direct *ep;
	int entryoffsetinblock;
	enum { NONE, COMPACT, FOUND } slotstatus;
	off_t slotoffset = -1;
	int slotsize;
	int slotfreespace;
	int slotneeded;
	int numdirpasses;
	off_t endsearch;
	off_t prevoff;
	int nlink = 0;
	struct inode *pdp;
	int i;
	int error;
	int lockparent;
	int isdotdot;
	int flag;
	off_t enduseful;
	char path[MAXPATHLEN];
	
	lockparent = ndp->ni_nameiop & LOCKPARENT;
	flag = ndp->ni_nameiop &~ (LOCKPARENT|FOLLOW);
	
	/* copy the name into the buffer */
	error = copyin(ndp->ni_dirp, path, MAXPATHLEN);
	if (error) {
		P.p_error = error;
		goto retNULL;
	}
	
	/* get starting directory */
	cp = path;
	if (*cp == '/') {
		while (*++cp == '/')
			;
		if ((dp = P.p_rdir) == NULL)
			dp = G.rootdir;
	} else
		dp = P.p_cdir;
	fs = dp->i_fs;
	ILOCK(dp);
	++dp->i_count;
	ndp->ni_endoff = 0;
	
dirloop:
	/* check accessibility of directory */
	if ((dp->i_mode&IFMT) != IFDIR) {
		P.p_error = ENOTDIR;
		goto bad;
	}
	if (access(dp, IEXEC))
		goto bad;
	
dirloop2:
	/* copy next component of name to ndp->ni_dent */
	for (i = 0; *cp != '\0' && *cp != '/'; ++cp) {
		if (i >= MAXNAMLEN) {
			P.p_error = ENAMETOOLONG;
			goto bad;
		}
		if (*cp & 0200)
			if ((*cp&0377) == ('/'|0200) || flag != DELETE) {
				P.p_error = EINVAL;
				goto bad;
			}
		ndp->ni_dent.d_name[i++] = *cp;
	}
	ndp->ni_dent.d_namlen = i;
	ndp->ni_dent.d_name[i] = '\0';
	isdotdot = (i == 2
	           && ndp->ni_dent.d_name[0] == '.'
	           && ndp->ni_dent.d_name[1] == '.');
	
	/* check for degenerate name ("/" or "") */
	if (ndp->ni_dent.d_name[0] == '\0') {
		if (flag != LOOKUP || lockparent) {
			P.p_error = EISDIR;
			goto bad;
		}
		goto retDP;
	}
	
	slotstatus = FOUND;
	if (flag == CREATE && *cp == '\0') {
		slotstatus = NONE;
		slotfreespace = 0;
		slotneeded = DIRSIZE(&ndp->ni_dent);
	}
	/* ... */
	endsearch = roundup(dp->i_size, DIRBLKSIZ);
	enduseful = 0;
	
searchloop:
	while (ndp->ni_offset < endsearch) {
		if (blkoff(ndp->ni_offset) == 0) {
			if (bp) {
				mapout(bp);
				brelse(bp);
			}
			bp = blkatoff(dp, ndp->ni_offset, (char **)NULL);
			if (!bp)
				goto bad;
			entryoffsetinblock = 0;
		}
		if (slotstatus == NONE && (entryoffsetinblock&(DIRBLKSIZ-1)) == 0) {
			slotoffset = -1;
			slotfreespace = 0;
		}
		
		mapout(bp);
		ep = (struct direct *)((char *)mapin(bp) + entryoffsetinblock);
		if (ep->d_reclen == 0 || /* XXX */
		   dirchk && dirbadentry(ep, entryoffsetinblock)) {
			dirbad(dp, ndp->ni_offset, "mangled entry");
			i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
			ndp->ni_offset += i;
			entryoffsetinblock += i;
			continue;
		}
		
		if (slotstatus != FOUND) {
			int size = ep->d_reclen; /* XXX */
			
			if (ep->d_ino != 0)
				size -= DIRSIZ(ep);
			if (size > 0) {
				if (size >= slotneeded) {
					slotstatus = FOUND;
					slotoffset = ndp->ni_offset;
					slotsize = ep->d_reclen; /* XXX */
				} else if (slotstatus == NONE) {
					slotfreespace += size;
					if (slotoffset == -1)
						slotoffset = ndp->ni_offset;
					if (slotfreespace >= slotneeded) {
						slotstatus = COMPACT;
						slotsize = ndp->ni_offset + ep->d_reclen - slotoffset; /* XXX */
					}
				}
			}
		}
		
		if (ep->d_ino) {
			if (ep->d_namlen == ndp->ni_dent.d_namlen && !bcmp(ndp->ni_dent.d_name, ep->d_name, ep->d_namlen))
				goto found;
		}
		prevoff = ndp->ni_offset;
		ndp->ni_offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
		if (ep->d_ino)
			enduseful = ndp->ni_offset;
	}
/* notfound: */
	if (numdirpasses == 2) {
		--numdirpasses;
		ndp->ni_offset = 0;
		endsearch = P.p_ncache.nc_prevoffset;
		goto searchloop;
	}
	
	if (flag == CREATE && *cp == '\0' && dp->i_nlink != 0) {
		if (access(dp, IWRITE))
			goto bad;
		if (slotstatus == NONE) {
			ndp->ni_offset = roundup(dp->i_size, DIRBLKSIZ);
			ndp->ni_count = 0;
			enduseful = ndp->ni_offset;
		} else {
			ndp->ni_offset = slotoffset;
			ndp->ni_count = slotsize;
			if (enduseful < slotoffset + slotsize)
				enduseful = slotoffset + slotsize;
		}
		ndp->ni_endoff = roundup(enduseful, DIRBLKSIZ);
		dp->i_flag |= IUPD|ICHG;
		if (bp) {
			mapout(bp);
			brelse(bp);
		}
		
		ndp->ni_pdir = dp;
		goto retNULL;
	}
	P.p_error = ENOENT;
	goto bad;
found:
	if (entryoffsetinblock + DIRSIZ(ep) > dp->i_size) {
		dirbad(dp, ndp->ni_offset, "i-size too small");
		dp->i_size = entryoffsetinblock + DIRSIZ(ep);
		dp->i_flag |= IUPD|ICHG;
	}
	
	ndp->ni_dent.d_ino = ep->d_ino;
	ndp->ni_dent.d_reclen = ep->d_reclen;
	mapout(bp);
	brelse(bp);
	bp = NULL;
	
	if (flag == DELETE && *cp == '\0') {
		if (access(dp, IWRITE))
			goto bad;
		ndp->ni_pdir = dp;
		
		if ((ndp->ni_offset & (DIRBLKSIZ-1)) == 0)
			ndp->ni_count = 0;
		else
			ndp->ni_count = ndp->ni_offset - prevoff;
		if (lockparent) {
			if (dp->i_number == ndp->ni_dent.d_ino)
				++dp->i_count;
			else {
				dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
				if (!dp) {
					iput(ndp->ni_pdir);
					goto bad;
				}
				
				if ((ndp->ni_pdir->i_mode & ISVTX) && P.p_uid != 0 && P.p_uid != ndp->ni_pdir->i_uid && dp->i_uid != P.p_uid) {
					iput(ndp->ni_pdir);
					P.p_error = EPERM;
					goto bad;
				}
			}
		}
		goto retDP;
	}
	
	if (isdotdot) {
		if (dp == P.p_rdir) {
			ndp->ni_dent.d_ino = dp->i_number;
		} else if (ndp->ni_dent.d_ino == ROOTINO && dp->i_number == ROOTINO) {
			struct mount *mp;
			dev_t d;
			d = dp->i_dev;
			for EACHMOUNT(mp) {
				if (mp->m_inodp && mp->m_dev == d) {
					iput(dp);
					dp = mp->m_inodp;
					ILOCK(dp);
					++dp->i_count;
					fs = dp->i_fs;
					cp -= 2;
					goto dirloop2;
				}
			}
		}
	}
	
	if ((flag == CREATE && lockparent) && *cp == '\0') {
		if (access(dp, IWRITE))
			goto bad;
		ndp->ni_pdir = dp;
		if (dp->i_number == ndp->ni_dent.d_ino) {
			P.p_error = EISDIR; /* XXX */
			goto bad;
		}
		dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
		if (!dp) {
			iput(ndp->ni_pdir);
			goto bad;
		}
		goto retDP;
	}
	
	pdp = dp;
	if (isdotdot) {
		IUNLOCK(pdp);
		dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
		if (!dp)
			goto bad2;
	} else if (dp->i_number == ndp->ni_dent.d_ino) {
		++dp->i_count;
	} else {
		dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
		IUNLOCK(pdp);
		if (!dp)
			goto bad2;
	}
	
haveino:
	fs = dp->i_fs;
	
	/* check for symbolic link */
	if ((dp->i_mode & IFMT) == IFLNK &&
		    ((ndp->ni_nameiop & FOLLOW) || *cp == '/')) {
		unsigned int pathlen = strlen(cp) + 1;
		
		if (dp->i_size + pathlen >= MAXPATHLEN - 1) {
			P.p_error = ENAMETOOLONG;
			goto bad2;
		}
		if (++nlink > MAXSYMLINKS) {
			P.p_error = ELOOP;
			goto bad2;
		}
		
		bp = bread(dp->i_dev, bmap(dp, 0, B_READ, 0));
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			bp = NULL;
			goto bad2;
		}
		
		bcopy(cp, path + (unsigned)dp->i_size, pathlen);
		bcopy(mapin(bp), path, (unsigned)dp->i_size);
		mapout(bp);
		brelse(bp);
		bp = NULL;
		cp = path;
		iput(dp);
		if (*cp == '/') {
			irele(pdp);
			while (*++cp == '/')
				;
			if ((dp = P.p_rdir) == NULL)
				dp = rootdir;
			ILOCK(dp);
			++dp->i_count;
		} else {
			dp = pdp;
			ILOCK(dp);
		}
		fs = dp->i_fs;
		goto dirloop;
	}
	
	/* not a symlink */
	if (*cp == '/') {
		while (*++cp == '/')
			;
		irele(pdp);
		goto dirloop;
	}
	if (lockparent)
		ndp->ni_pdir = pdp;
	else
		irele(pdp);
	
retDP:
	ndp->ni_ip = dp;
	return dp;
	
bad2:
	irele(pdp);
bad:
	if (bp) {
		mapout(bp);
		brelse(bp);
	}
	if (dp)
		iput(dp);
retNULL:
	ndp->ni_ip = NULL;
	return NULL;
}

