#ifndef _PWD_H_
#define _PWD_H_

/* $Id$ */

/* more or less POSIX compliant */

#include <sys/types.h>

struct passwd {
	char	*pw_name;	/* Username */
	uid_t	pw_uid; 	/* User ID */
	gid_t	pw_gid; 	/* Group ID */
	char	*pw_dir;	/* Home directory */
	char	*pw_shell;	/* Shell program */
	char	*pw_passwd;	/* Password */	/* not in POSIX */
	char	*pw_gecos;	/* Real name */	/* not in POSIX */
};

extern struct passwd	*getpwnam(char *__name);
extern struct passwd	*getpwuid(int __uid);
extern void		endpwent(void);
extern struct passwd	*getpwent(void);
extern void		setpwent(void);

/* not in POSIX
extern int putpwent(struct passwd * __p, FILE * __f);
extern int getpw(int uid, char *buf);

extern struct passwd *fgetpwent(FILE *file);


extern struct passwd * __getpwent(int passwd_fd);

extern char *_path_passwd;

#define getlogin()	getpwnam(getuid())
*/

#endif /* _PWD_H_ */
