#ifndef _GRP_H_
#define _GRP_H_

/* $Id$ */

/* should be POSIX compliant */

#include <sys/types.h>

/* FIXME */
#define GR_MAX_GROUPS	32
#define GR_MAX_MEMBERS	16

struct group {
	char	*gr_name;	/* Group name.	*/
	gid_t	gr_gid; 	/* Group ID.	*/
	char	**gr_mem;	/* Member list. */
#if 0
	char	*gr_passwd;	/* Password.	*/
#endif
};

extern struct group	*getgrgid(gid_t __gid);
extern struct group	*getgrnam(const char *__name);

/* [XSI] */
extern struct group	*getgrent(void);
extern void	endgrent(void);
extern void	setgrent(void);

/* not in POSIX */
/*
extern struct group *fgetgrent(FILE *__file);

extern int setgroups(size_t __n, int *__groups);
extern int initgroups(char *__user, int __gid);

extern struct group *__getgrent(int __grp_fd);

extern char *_path_group;
*/

#endif /* _GRP_H_ */
