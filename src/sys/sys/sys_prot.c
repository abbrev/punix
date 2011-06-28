/*
 * Punix, Puny Unix kernel
 * Copyright 2005, 2007 Chris Williams
 * 
 * $Id$
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

#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>

#include "proc.h"
#include "file.h"
#include "punix.h"
#include "process.h"
/*
#include "uio.h"
*/
#include "inode.h"
#include "queue.h"
#include "globals.h"

/* POSIX.1 */
STARTUP(void sys_getuid())
{
	P.p_retval = P.p_ruid;
}

/* POSIX.1 */
STARTUP(void sys_geteuid())
{
	P.p_retval = P.p_euid;
}

/* POSIX.1 */
STARTUP(void sys_getgid())
{
	P.p_retval = P.p_rgid;
}

/* POSIX.1 */
STARTUP(void sys_getegid())
{
	P.p_retval = P.p_egid;
}

struct groupsa {
	int size;
	gid_t *list;
};

/* POSIX.1 */
STARTUP(void sys_getgroups())
{
	struct groupsa *ap = (struct groupsa *)P.p_arg;
	gid_t *gp;
	int size;
	
	for (gp = &P.p_groups[0];
	     gp < &P.p_groups[NGROUPS] && *gp != NOGROUP;
	     ++gp)
		;
	
	/* return the actual number of group ID's */
	P.p_retval = size = gp - P.p_groups;
	
	/* if size is 0, return the actual number of groups IDs in the list */
	if (ap->size == 0)
		return;
	
	if (ap->size < size) {
		P.p_error = EINVAL;
		return;
	}
	
	P.p_error = copyout(ap->list, P.p_groups, size*sizeof(P.p_groups[0]));
}

/* XXX: check the following set functions for correctness and security */

#if 1
STARTUP(static int validuid(uid_t uid))
{
	if (uid > 0)
		return 1;
	
	P.p_error = EINVAL;
	return 0;
}
#else
#define validuid(uid) ((uid) > 0 ? 1 : (P.p_error = EINVAL, 0))
#endif

#if 0
STARTUP(static int validgid(gid_t gid))
{
	return validuid(gid);
}
#else
#define validgid(gid) validuid(gid)
#endif

/* POSIX.1 */
STARTUP(void sys_setuid())
{
	struct a {
		uid_t uid;
	} *ap = (struct a *)P.p_arg;
	uid_t uid = ap->uid;
	
	if (!validuid(uid))
		return;
	
	if (0 == P.p_euid)
		P.p_ruid = P.p_euid = P.p_svuid = uid;
	else if (uid == P.p_ruid || uid == P.p_euid || uid == P.p_svuid)
		P.p_euid = uid;
	else
		P.p_error = EPERM;
}

/* POSIX.1 */
STARTUP(void sys_setgid())
{
	struct a {
		gid_t gid;
	} *ap = (struct a *)P.p_arg;
	gid_t gid = ap->gid;
	
	if (!validgid(gid))
		return;
	
	if (0 == P.p_euid)
		P.p_rgid = P.p_egid = P.p_svgid = gid;
	else if (gid == P.p_rgid || gid == P.p_egid || gid == P.p_svgid)
		P.p_egid = gid;
	else
		P.p_error = EPERM;
}

STARTUP(int suser())
{
        if (P.p_euid == 0)
                return 1;

        P.p_error = EPERM;
        return 0;
}

/* XXX not in POSIX.1 */
STARTUP(void sys_setgroups())
{
	struct groupsa *ap = (struct groupsa *)P.p_arg;
	
	if (!suser())
		return;
	
	if (ap->size > sizeof(P.p_groups) / sizeof(P.p_groups[0])) {
		P.p_error = EINVAL;
		return;
	}
	
	P.p_error =
	 copyin(P.p_groups, ap->list, ap->size * sizeof(P.p_groups[0]));
	
	if (P.p_error)
		return;
	
	if (ap->size < NGROUPS)
		P.p_groups[ap->size] = NOGROUP;
}

/* POSIX.1 */
STARTUP(void sys_seteuid())
{
	struct a {
		uid_t euid;
	} *ap = (struct a *)P.p_arg;
	uid_t uid = ap->euid;
	
	if (!validuid(uid))
		return;
	
	if (uid == P.p_ruid || uid == P.p_euid || uid == P.p_svuid || suser())
		P.p_euid = uid;
}

/* POSIX.1 */
STARTUP(void sys_setegid())
{
	struct a {
		gid_t egid;
	} *ap = (struct a *)P.p_arg;
	uid_t gid = ap->egid;
	
	if (!validgid(gid))
		return;
	
	if (gid == P.p_rgid || gid == P.p_egid || gid == P.p_svgid || suser())
		P.p_egid = gid;
}

/* XSI */
STARTUP(void sys_setreuid())
{
	struct a {
		uid_t ruid, euid;
	} *ap = (struct a *)P.p_arg;
	uid_t ruid = ap->ruid, euid = ap->euid;
	uid_t oldruid = P.p_ruid;
	
	if (ruid != -1 && validuid(ruid)
	&& (ruid == P.p_ruid || ruid == P.p_euid || suser()))
		P.p_ruid = ruid;
	
	if (euid != -1 && validuid(euid)
	&& (euid == oldruid || euid == P.p_euid || euid == P.p_svuid || suser()))
		P.p_euid = euid;
}

/* XSI */
STARTUP(void sys_setregid())
{
	struct a {
		gid_t rgid, egid;
	} *ap = (struct a *)P.p_arg;
	gid_t rgid = ap->rgid, egid = ap->egid;
	gid_t oldrgid = P.p_rgid;
	
	if (rgid != -1 && validgid(rgid)
	&& (rgid == P.p_svgid || rgid == P.p_rgid || suser()))
		P.p_rgid = rgid;
	
	if (egid != -1 && validgid(egid)
	&& (egid == oldrgid || egid == P.p_egid || egid == P.p_svgid || suser()))
		P.p_egid = egid;
}

/* BSD */
STARTUP(void sys_setresuid())
{
	struct a {
		uid_t ruid, euid, suid;
	} *ap = (struct a *)P.p_arg;
	uid_t ruid = ap->ruid, euid = ap->euid, suid = ap->suid;
	uid_t oldruid = P.p_ruid;
	uid_t oldeuid = P.p_euid;
	uid_t oldsuid = P.p_svuid;
	
	if (ruid != -1 && validuid(ruid) &&
	    (ruid == oldruid || ruid == oldeuid || ruid == oldsuid || suser()))
		P.p_ruid = ruid;
	
	if (euid != -1 && validuid(euid) &&
	    (euid == oldruid || euid == oldeuid || euid == oldsuid || suser()))
		P.p_euid = euid;
	
	if (suid != -1 && validuid(suid) &&
	    (suid == oldruid || suid == oldeuid || suid == oldsuid || suser()))
		P.p_svuid = suid;
}

/* BSD */
STARTUP(void sys_setresgid())
{
	struct a {
		gid_t rgid, egid, sgid;
	} *ap = (struct a *)P.p_arg;
	gid_t rgid = ap->rgid, egid = ap->egid, sgid = ap->sgid;
	gid_t oldrgid = P.p_rgid;
	gid_t oldegid = P.p_egid;
	gid_t oldsgid = P.p_svgid;
	
	if (rgid != -1 && validgid(rgid) &&
	    (rgid == oldrgid || rgid == oldegid || rgid == oldsgid || suser()))
		P.p_rgid = rgid;
	
	if (egid != -1 && validgid(egid) &&
	    (egid == oldrgid || egid == oldegid || egid == oldsgid || suser()))
		P.p_egid = egid;
	
	if (sgid != -1 && validgid(sgid) &&
	    (sgid == oldrgid || sgid == oldegid || sgid == oldsgid || suser()))
		P.p_svgid = sgid;
}

/*
 * Look up a pathname and test if the resultant inode is owned by the
 * current user. If not, try for super-user.
 * If permission is granted, return inode pointer.
 */
static struct inode *owner(const char *path)
{
	struct inode *ip = NULL;

#ifdef NOTYET /*XXX*/
	ip = namei(path, 0);
#endif
	if (ip == NULL || P.p_euid == ip->i_uid || suser())
		return ip;
	
	/* we're not the owner of the file nor the superuser :( */
	i_unref(ip);
	return NULL;
}

void sys_chmod()
{
	/* int chmod(const char *path, mode_t mode); */
	struct a {
		const char *path;
		mode_t mode;
	} *ap = (struct a *)P.p_arg;
	struct inode *ip;
	mode_t mode;
	
	ip = owner(ap->path);
	if (!ip) return;
	ip->i_mode &= ~07777;
	mode = ap->mode;
	if (P.p_euid)
		mode &= ~S_ISVTX;
	ip->i_mode |= mode & 07777;
	ip->i_flag |= ICHG;
#if 0
	if (ip->i_flag&ITEXT && !(ip->i_mode&S_ISVTX))
		xrele(ip);
#endif
	i_unref(ip);
}

/* FIXME: make this function less spaghetti-like */
void sys_chown()
{
	struct a {
		const char *path;
		uid_t owner;
		gid_t group;
	} *ap = (struct a *)P.p_arg;
	struct inode *ip;
	
	if (!(ip = owner(ap->path)))
		return;
	
	if (ap->owner == (uid_t)-1 && ap->group == (gid_t)-1)
		return;
	
	if (!P.p_euid) {
		goto chown;
	} else if (ap->owner == P.p_euid || ap->owner == (uid_t)-1) {
		gid_t *gp;
		if (ap->group == (gid_t)-1)
			goto chown;
		else {
			/* make sure the new group is our effective group id
			 * or is one of our supplementary groups */
			if (ap->group == P.p_egid) goto chown;
			for (gp = &P.p_groups[0];
			     gp < &P.p_groups[NGROUPS] && *gp != (gid_t)-1;
			     ++gp) {
				if (*gp == ap->group) goto chown;
			}
		}
	}
	P.p_error = EPERM;
	goto out;
	
chown:
	if (ap->owner != (uid_t)-1) ip->i_uid = ap->owner;
	if (ap->group != (gid_t)-1) ip->i_gid = ap->group;
	ip->i_flag |= ICHG;
out:
	i_unref(ip);
}

