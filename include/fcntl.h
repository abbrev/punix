#ifndef _FCNTL_H_
#define _FCNTL_H_

/* $Id$ */

/* mostly POSIX compliant */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* mask for use with file access modes: */
#define O_ACCMODE  0003	/* mask for file access modes */
/* file access modes used for open(2) and fcntl(2): */
#define O_RDONLY   0001	/* open for reading only */
#define O_WRONLY   0002	/* open for writing only */
#define O_RDWR     0003	/* open for reading and writing */

/* file creation flags for the oflag value to open(2): */
#define O_CREAT    0004	/* create file if it does not exist */
#define O_EXCL     0010	/* exclusive use flag */
#define O_NOCTTY   0020	/* do not assign controlling terminal */
#define O_TRUNC    0040	/* truncate flag */
#define O_APPEND   0100	/* set append mode */
#define O_NONBLOCK 0200	/* non-blocking mode */
#define O_SYNC     0400	/* write according to synchronized I/O file */
                       	/* integrity completion */

/* values for cmd used by fcntl(2): */
#define F_DUPFD  0	/* duplicate file descriptor */
#define F_GETFD  1	/* get file descriptor flags */
#define F_SETFD  2	/* set file descriptor flags */
#define F_GETFL  3	/* get file status flags and file access modes */
#define F_SETFL  4	/* set file flags */
#define F_GETLK  5	/* get record locking information */
#define F_SETLK  6	/* set record locking information */
#define F_SETLKW 7	/* set record locking information; wait if blocked */
#define F_GETOWN 8	/* get PID or PGID to receive SIGURG signals */
#define F_SETOWN 9	/* set PID or PGID to receive SIGURG signals */

/* file descriptor flags used for fcntl(): */
#define FD_CLOEXEC 1

/* values for l_type used for record locking with fcntl(): */
#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2

/* a file lock */
struct flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;
	pid_t	l_pid;
};

int	creat(const char *__path, mode_t __mode);
int	fcntl(int __fd, int __cmd, ...);
int	open(const char *__path, int __flags, ...);

#endif /* _FCNTL_H_ */
