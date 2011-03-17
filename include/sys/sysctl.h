#ifndef _SYS_SYSCTL_H_
#define _SYS_SYSCTL_H_

enum {
	CTL_HW,
	CTL_KERN,
	CTL_VM,
	CTL_MACHDEP,
	CTL_VFS,
	CTL_USER,
	CTL_NET,
	CTL_DEBUG,
};

enum {
	HW_MACHINE,
	HW_MODEL,
	HW_NCPU,
	HW_BYTEORDER,
	HW_PHYSMEM,
	HW_USERMEM,
	HW_PAGESIZE,
	HW_FLOATINGPT,
	HW_MACHINE_ARCH,
	HW_REALMEM
};

enum {
	KERN_ARGMAX,
	KERN_BOOTFILE,
	KERN_BOOTTIME,
	KERN_CLOCKRATE,
	KERN_FILE,
	KERN_HOSTID,
	KERN_HOSTUUID,
	KERN_HOSTNAME,
	KERN_JOB_CONTROL,
	KERN_MAXFILES,
	KERN_MAXFILESPERPROC,
	KERN_MAXPROC,
	KERN_MAXPROCPERUID,
	KERN_MAXVNODES,
	KERN_NGROUPS,
	KERN_NISDOMAINNAME,
	KERN_OSRELDATE,
	KERN_OSRELEASE,
	KERN_OSREV,
	KERN_OSTYPE,
	KERN_POSIX1,
	KERN_PROC,
	KERN_PROF,
	KERN_QUANTUM,
	KERN_SAVED_IDS,
	KERN_SECURELVL,
	KERN_UPDATEINTERVAL,
	KERN_UPTIME,
	KERN_VERSION,
	KERN_VNODE,
};

enum {
	KERN_PROC_ALL,
	KERN_PROC_PID,
	KERN_PROC_PGRP,
	KERN_PROC_TTY,
	KERN_PROC_UID,
	KERN_PROC_RUID,
};

/*
enum {
	MACHDEP_???;
};
*/

enum {
	USER_BC_BASE_MAX,
	USER_BC_DIM_MAX,
	USER_BC_SCALE_MAX,
	USER_BC_STRING_MAX,
	USER_COLL_WEIGHTS_MAX,
	USER_CS_PATH,
	USER_EXPR_NEST_MAX,
	USER_LINE_MAX,
	USER_POSIX2_CHAR_TERM,
	USER_POSIX2_C_BIND,
	USER_POSIX2_C_DEV,
	USER_POSIX2_FORT_DEV,
	USER_POSIX2_FORT_RUN,
	USER_POSIX2_LOCALEDEF,
	USER_POSIX2_SW_DEV,
	USER_POSIX2_UPE,
	USER_POSIX2_VERSION,
	USER_RE_DUP_MAX,
	USER_STREAM_MAX,
	USER_TZNAME_MAX,
};

#define CTL_MAXNAME 5 /* ? */

/*
 * top/ps/etc need at least the following fields:
 * priority
 * virtual mem?
 * resident mem
 * shared mem
 * state (sleep, run, etc)
 * code size
 * data size
 * wchan (wait channel)
 *
 * according to POSIX.1 page on ps command:
 * ruser        real uid
 * user         effective uid
 * rgroup       real group id
 * group        effective group id
 * pid          process id
 * ppid         parent process id
 * pgid         process group id
 * pcpu         %CPU
 * vsz          size of process in 1024 byte units
 * nice         nice value
 * etime        elapsed time since the process was started
 * time         cumulative CPU time
 * tty          controlling tty
 * comm         argv[0]
 * args         argv[1..n]
 *
 */
struct kinfo_proc {
	pid_t kp_pid;

	pid_t kp_pgid;
	pid_t kp_ppid;

	uid_t kp_euid;
	uid_t kp_ruid;
	uid_t kp_svuid;
	
	gid_t kp_egid;
	gid_t kp_rgid;
	gid_t kp_svgid;

	size_t kp_vsz;
	int kp_pri;
	int kp_nice;
	int kp_state;
	unsigned kp_pcpu;
	clock_t kp_ctime; /* cumulative cpu time */
	clock_t kp_cctime; /* cumulative children cpu time */
	time_t kp_stime; /* start time */
	dev_t kp_tty;
#define MAXCMDLEN 15
	char kp_cmd[MAXCMDLEN+1];
};

#define PRUN     0
#define PSLEEP   1
#define PDSLEEP  2
#define PSTOPPED 3
#define PZOMBIE  4

/*
 * 'R' = running
 * 'S' = sleeping
 * 'D' = uninterruptible sleep
 * 'T' = traced or stopped
 * 'Z' = zombie
 */



#endif
