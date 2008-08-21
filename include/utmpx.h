#ifndef _UTMPX_H_
#define _UTMPX_H_

#include <sys/types.h>
#include <sys/time.h>

struct utmpx {
	char	ut_user[];	/* user login name */
	char	ut_id[];	/* unspecified initialisation process identifier */
	char	ut_line[];	/* device name */
	pid_t	ut_pid;		/* process ID */
	short	ut_type;	/* type of entry */
	struct timeval	ut_tv;	/* time entry was made */
};

/* possible values for utmpx::ut_type */
#define EMPTY		0
#define BOOT_TIME	1
#define OLD_TIME	2
#define NEW_TIME	3
#define USER_PROCESS	4
#define INIT_PROCESS	5
#define LOGIN_PROCESS	6
#define DEAD_PROCESS	7

void		endutxent(void);
struct utmpx	*getutxent(void);
struct utmpx	tutxid(const struct utmpx *__ut);
struct utmpx	tutxline(const struct utmpx *__ut);
struct utmpx	*pututxline(const struct utmpx *__ut);
void		setutxent(void);

#endif
