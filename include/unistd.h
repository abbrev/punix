#ifndef _UNISTD_H_
#define _UNISTD_H_

/* $Id$ */

#include <sys/types.h>
#include <inttypes.h>

/* we don't conform (yet) */
/*
#define _POSIX_VERSION     200112L
#define _POSIX2_VERSION    200112L
#define _POSIX_JOB_CONTROL 1
#define _POSIX_NO_TRUNC    1
#define _POSIX_REGEXP      1
#define _POSIX_SAVED_IDS   1
#define _POSIX_SHELL       1
#define _POSIX_VDISABLE    ????
#define _POSIX2_C_BIND     200112L
#define _POSIX2_CHAR_TERM  1

*/

#define R_OK 4 /* Test for read permission.  */
#define W_OK 2 /* Test for write permission.  */
#define X_OK 1 /* Test for execute (search) permission.  */
#define F_OK 0 /* Test for existence of file. */

#define SEEK_SET 0 /* Set file offset to offset. */
#define SEEK_CUR 1 /* Set file offset to current plus offset. */
#define SEEK_END 2 /* Set file offset to EOF plus offset. */

/* what values do these have?
#define F_LOCK 
#define F_TEST 
#define F_TLOCK 
#define F_ULOCK 
*/

#define STDIN_FILENO  0 /* File number of stdin */
#define STDOUT_FILENO 1 /* File number of stdout */
#define STDERR_FILENO 2 /* File number of stderr */

int access(const char *__path, int __amode);
unsigned alarm(unsigned __seconds);
int chdir(const char *__path);
int chown(const char *__path, uid_t __owner, gid_t __group);
int close(int __fd);
size_t confstr(int __name, char *__buf, size_t __len);

/*
[XSI][Option Start]
char *crypt(const char *, const char *);
char *ctermid(char *);
[Option End]
*/

int dup(int __fd);
int dup2(int __fd, int __fd2);

/*
[XSI][Option Start]
void encrypt(char[64], int);
[Option End]
*/

int execl(const char *__path, const char *__arg0, ...);
int execle(const char *__path, const char *__arg0, ...);
int execlp(const char *__file, const char *__arg0, ...);
int execv(const char *__path, char *const __argv[]);
int execve(const char *__path, char *const __argv[],
 char *const __envp[]);
int execvp(const char *__file, char *const __argv[]);
void _exit(int __status);
int fchown(int __fd, uid_t __owner, gid_t __group);

/*
[XSI][Option Start]
int fchdir(int);
[Option End]
[SIO][Option Start]
int fdatasync(int);
[Option End]
*/

pid_t fork(void);
long fpathconf(int __fd, int __name);

/*
[FSC][Option Start]
int fsync(int);
[Option End]
*/

int ftruncate(int __fd, off_t __len);
char *getcwd(char *__buf, size_t __size);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int __gidsetsize, gid_t __grouplist[]);

/*
[XSI][Option Start]
long gethostid(void);
[Option End]
*/

int gethostname(char *__name, size_t __len);
char *getlogin(void);
int getopt(int __argc, char * const __argv[],
 const char *__optstring);

/*
[XSI][Option Start]
pid_t getpgid(pid_t __pgid);
[Option End]
*/

pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);

/*
[XSI][Option Start]
pid_t getsid(pid_t __pgid);
[Option End]
*/

uid_t getuid(void);

/*
[XSI][Option Start]
char *getwd(char *); (LEGACY )
[Option End]
*/

int isatty(int);

/*
[XSI][Option Start]
int lchown(const char *, uid_t, gid_t);
[Option End]
*/

int link(const char *__path1, const char *__path2);

/*
[XSI][Option Start]
int lockf(int, int, off_t);
[Option End]
*/

off_t lseek(int __fd, off_t __offset, int __whence);

/*
[XSI][Option Start]
*/
int nice(int incr);
/*
[Option End]
*/

long pathconf(const char *__path, int __name);
int pause(void);
int pipe(int __fd[2]);

/*
[XSI][Option Start]
ssize_t pread(int, void *, size_t, off_t);
ssize_t pwrite(int, const void *, size_t, off_t);
[Option End]
*/

ssize_t read(int __fd, void *__buf, size_t __nbyte);
ssize_t readlink(const char *__path, char *__buf, size_t __bufsize);
int rmdir(const char *__path);
int setegid(gid_t __gid);
int seteuid(uid_t __uid);
int setgid(gid_t __gid);
int setpgid(pid_t __pid, pid_t __pgid);

/*
[XSI][Option Start]
pid_t setpgrp(void);
int setregid(gid_t, gid_t);
int setreuid(uid_t, uid_t);
[Option End]
*/

pid_t setsid(void);
int setuid(uid_t __uid);
unsigned sleep(unsigned __seconds);

/*
[XSI][Option Start]
void swab(const void *, void *, ssize_t);
[Option End]
*/

int symlink(const char *__path1, const char *__path2);

/*
[XSI][Option Start]
void sync(void);
[Option End]
*/

long sysconf(int __name);
pid_t tcgetpgrp(int __fd);
int tcsetpgrp(int __fd, pid_t __pgid);

/*
[XSI][Option Start]
int truncate(const char *, off_t);
[Option End]
*/

char *ttyname(int __fd);
int ttyname_r(int __fd, char *__name, size_t __namesize);

/*
[XSI][Option Start]
useconds_t ualarm(useconds_t, useconds_t);
[Option End]
*/

int unlink(const char *__path);

/*
[XSI][Option Start]
int usleep(useconds_t __useconds);
pid_t vfork(void);
[Option End]
*/
/* NB: vfork() *must* be inline assembly because returning from a function call
 * would invalidate the stack where the return address is located, and that
 * would have to be preserved for the parent process to return from vfork (if
 * it were a function call).
 */
static inline int vfork()
{
	int result;
	asm volatile("\n"
		"	move #66,%%d0 \n"
		"	trap #0 \n"
		"	bcc 0f \n"
		"	jbsr cerror \n"
		"0:	move %%d0,%0 \n"
		: "=r"(result)
		:
		: "d0", "d1", "d2", "a0", "a1"); /* caller-saved registers */
	return result;
}

ssize_t		write(int __fd, const void *__buf, size_t __nbyte);

extern char	*optarg;
extern int	optind, opterr, optopt;

#endif /* _UNISTD_H_ */
