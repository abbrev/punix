#include "sysent.h"
#include "punix.h"

/* $Id$ */


void sys_NONE();
void sys_sigreturn();
void sys_exit();
void sys_fork();
void sys_vfork();
void sys_wait();
void sys_waitpid();
void sys_wait3();
void sys_wait4();
void sys_getrusage();

void sys_getitimer();
void sys_setitimer();
void sys_gettimeofday();
void sys_settimeofday();
void sys_time();
void sys_getloadavg1();

void sys_read();
void sys_write();
void sys_open();
void sys_creat();
void sys_close();
void sys_pipe();
void sys_dup();
void sys_dup2();
void sys_lseek();
void sys_umask();
void sys_fstat();
void sys_ioctl();
void sys_fcntl();
void sys_utime();

void sys_getpid();
void sys_getppid();
void sys_getuid();
void sys_getgid();
void sys_geteuid();
void sys_getegid();
void sys_setuid();
void sys_setgid();
void sys_seteuid();
void sys_setegid();
void sys_setreuid();
void sys_setregid();
void sys_getgroups();
void sys_setgroups();
void sys_nice();
void sys_getpriority();
void sys_setpriority();

void sys_execve();

void sys_getdtablesize();

#define sys_link	sys_NONE
#define sys_unlink	sys_NONE
#define sys_chdir	sys_NONE
#define sys_fchdir	sys_NONE
#define sys_mknod	sys_NONE
#define sys_chmod	sys_NONE
#define sys_chown	sys_NONE
#define sys_mount	sys_NONE
#define sys_umount	sys_NONE
#define sys_sigaction	sys_NONE
#define sys_sigprocmask	sys_NONE
#define sys_access	sys_NONE
#define sys_sigpending	sys_NONE
#define sys_sigaltstack	sys_NONE
#define sys_sync	sys_NONE
#define sys_kill	sys_NONE
#define sys_stat	sys_NONE
#define sys_getlogin	sys_NONE
#define sys_lstat	sys_NONE
#define sys_setlogin	sys_NONE
#define sys_reboot	sys_NONE
#define sys_symlink	sys_NONE
#define sys_readlink	sys_NONE
#define sys_chroot	sys_NONE
#define sys_getpgrp	sys_NONE
#define sys_setpgrp	sys_NONE
#define sys_select	sys_NONE
#define sys_fsync	sys_NONE
#define sys_fchown	sys_NONE
#define sys_fchmod	sys_NONE
#define sys_rename	sys_NONE
#define sys_truncate	sys_NONE
#define sys_ftruncate	sys_NONE
#define sys_flock	sys_NONE
#define sys_shutdown	sys_NONE
#define sys_mkdir	sys_NONE
#define sys_rmdir	sys_NONE
#define sys_adjtime	sys_NONE

STARTUP(struct sysent sysent[]) = {
	{0, sys_NONE},		/* 0 */
	{1, sys_exit},
	{0, sys_fork},
	{5, sys_read},
	{5, sys_write},
	{4, sys_open},
	{1, sys_close},
	{0, sys_NONE}, /* sys_wait4 */
	{0, sys_NONE},
	{4, sys_link},
	{2, sys_unlink},		/* 10 */
	{4, sys_NONE},
	{2, sys_chdir},
	{1, sys_fchdir},
	{4, sys_mknod},
	{3, sys_chmod},
	{4, sys_chown},
	{0, sys_NONE},
	{0, sys_NONE},
	{4, sys_lseek},
	{0, sys_getpid},	/* 20 */
	{0, sys_mount},		/* XXX */
	{0, sys_umount},	/* XXX */
	{0, sys_NONE},
	{0, sys_getuid},
	{0, sys_geteuid},
	{0, sys_NONE},
	{0, sys_getppid},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},		/* 30 */
	{5, sys_sigaction},
	{5, sys_sigprocmask},
	{3, sys_access},
	{1, sys_nice},
	{2, sys_sigpending},
	{0, sys_sync},
	{2, sys_kill},
	{4, sys_stat},
	{0, sys_getlogin},
	{4, sys_lstat},		/* 40 */
	{1, sys_dup},
	{0, sys_pipe},
	{0, sys_setlogin},	/* ??? */
	{0, sys_NONE},
	{1, sys_setuid},
	{1, sys_seteuid},
	{0, sys_getgid},
	{0, sys_getegid},
	{1, sys_setgid},
	{1, sys_setegid},	/* 50 */
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{4, sys_ioctl},
	{3, sys_reboot},
	{0, sys_NONE},
	{4, sys_symlink},
	{6, sys_readlink},
	{6, sys_execve},
	{1, sys_umask},		/* 60 */
	{2, sys_chroot},
	{3, sys_fstat},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_vfork},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},		/* 70 */
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{3, sys_getgroups},
	{4, sys_setgroups},	/* 80 */
	{0, sys_getpgrp},
	{0, sys_setpgrp},
	{0, sys_setitimer},
	{0, sys_NONE}, /* sys_wait3 */
	{0, sys_NONE},
	{3, sys_getitimer},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_getdtablesize},
	{2, sys_dup2},		/* 90 */
	{0, sys_NONE},
	{4, sys_fcntl},
	{9, sys_select},
	{0, sys_NONE},
	{1, sys_fsync},
	{2, sys_getpriority},
	{3, sys_setpriority},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},		/* 100 */
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_sigreturn},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},		/* 110 */
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{4, sys_gettimeofday},
	{3, sys_getrusage},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},		/* 120 */
	{0, sys_NONE},
	{4, sys_settimeofday},
	{3, sys_fchown},
	{2, sys_fchmod},
	{0, sys_NONE},
	{2, sys_setreuid},
	{2, sys_setregid},
	{4, sys_rename},
	{4, sys_truncate},
	{3, sys_ftruncate},	/* 130 */
	{2, sys_flock},
	{0, sys_NONE},
	{1, sys_NONE},
	{2, sys_shutdown},
	{0, sys_NONE},
	{3, sys_mkdir},
	{2, sys_rmdir},
	{4, sys_utime},
	{4, sys_NONE},
	{4, sys_adjtime},	/* 140 */
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},		/* 150 */
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{0, sys_NONE},
	{3, sys_getloadavg1},		/* 160 */
};

const int nsysent = sizeof(sysent) / sizeof(struct sysent);

#if 0
STARTUP(const char *sysname[]) = {
	"NONE",		/* 0 */
	"exit",
	"fork",
	"read",
	"write",
	"open",
	"close",
	"NONE",
	"NONE",
	"NONE",
	"NONE",		/* 10 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"lseek",
	"getpid",	/* 20 */
	"NONE",
	"NONE",
	"NONE",
	"getuid",
	"geteuid",
	"NONE",
	"getppid",
	"NONE",
	"NONE",
	"NONE",		/* 30 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",		/* 40 */
	"dup",
	"NONE",
	"NONE",
	"NONE",
	"setuid",
	"seteuid",
	"getgid",
	"getegid",
	"setgid",
	"setegid",	/* 50 */
	"NONE",
	"NONE",
	"NONE",
	"ioctl",
	"fcntl",
	"NONE",
	"NONE",
	"NONE",
	"execve",
	"umask",	/* 60 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"vfork",
	"NONE",
	"NONE",
	"NONE",
	"NONE",		/* 70 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"getgroups",
	"setgroups",	/* 80 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"getdtablesize",
	"dup2",		/* 90 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",		/* 100 */
	"NONE",
	"NONE",
	"sigreturn",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",		/* 110 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",		/* 120 */
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"NONE",
	"setreuid",
	"setregid",
	"NONE",
	"NONE",
	"NONE",		/* 130 */
};
#endif
