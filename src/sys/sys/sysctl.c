#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <string.h>

#include "punix.h"
#include "globals.h"
#include "proc.h"

#if 0
/*
 * taken and modified from 4.4BSD-Lite2. TODO: make this work in Punix!
 */
typedef long segsz_t;

struct ucred {
	uid_t   cr_uid;                 /* effective user id */
	short   cr_ngroups;             /* number of groups */
	gid_t   cr_groups[NGROUPS];     /* groups */
};

struct  pcred {
	uid_t   p_ruid;                 /* Real user id. */
	uid_t   p_svuid;                /* Saved effective user id. */
	gid_t   p_rgid;                 /* Real group id. */
	gid_t   p_svgid;                /* Saved effective group id. */
};
struct  session {
	struct  proc *s_leader;         /* Session leader. */
	struct  vnode *s_ttyvp;         /* Vnode of controlling terminal. */
	struct  tty *s_ttyp;            /* Controlling terminal. */
	char    s_login[MAXLOGNAME];    /* Setlogin() name. */
};

struct kinfo_proc {
	struct  proc kp_proc;                   /* proc structure */
	struct  eproc {
		struct  session *e_sess;        /* session pointer */
		struct  pcred e_pcred;          /* process credentials */

		struct  ucred e_ucred;          /* current credentials */
#ifdef sparc
		struct {
			segsz_t vm_rssize;      /* resident set size */
			segsz_t vm_tsize;       /* text size */
			segsz_t vm_dsize;       /* data size */
			segsz_t vm_ssize;       /* stack size */
		} e_vm;
#else
		struct  vmspace e_vm;           /* address space */
#endif
		pid_t   e_ppid;                 /* parent process id */
		pid_t   e_pgid;                 /* process group id */
		short   e_jobc;                 /* job control counter */
		dev_t   e_tdev;                 /* controlling tty dev */
		pid_t   e_tpgid;                /* tty process group id */
		struct  session *e_tsess;       /* tty session pointer */
#define WMESGLEN        7
		char    e_wmesg[WMESGLEN+1];    /* wchan message */
		segsz_t e_xsize;                /* text size */
		short   e_xrssize;              /* text rss */
		short   e_xccount;              /* text references */
		short   e_xswrss;
		long    e_flag;
#define EPROC_CTTY      0x01    /* controlling tty vnode active */
#define EPROC_SLEADER   0x02    /* session leader */
		char    e_login[MAXLOGNAME];    /* setlogin() name */
		long    e_spare[4];
	} kp_eproc;
};
#endif

struct sysctl_entry {
	const char *name;
	enum {
		CTL_UNSUPPORTED,
		CTL_TABLE,
		CTL_INTEGER,
		CTL_INTEGERP,
		CTL_STRING,
		CTL_FUNCTION,
	} type;
	union {
		const struct sysctl_table *table;
		long integer;
		long *integerp;
		char *string;
		int (*function)(int *, unsigned, void *, size_t *);
	} get;
	union {
		const struct sysctl_table *table;
		int (*function)(int *, unsigned, void *, size_t);
	} set;
	//void *arg;
};

struct sysctl_table {
	int size;
	struct sysctl_entry entries[];
};

static int putvalue(void *oldp, size_t *oldlenp, void *value, size_t valuelen)
{
	if (oldp) {
		if (*oldlenp < valuelen) return ENOMEM;
		if (copyout(oldp, value, valuelen))
			return EFAULT;
	}
	*oldlenp = valuelen;
	return 0;
}

static void proc_to_kinfo_proc(struct proc *pp, struct kinfo_proc *kp)
{
	/* TODO */
#if 0
	pid_t kp_pid;

	pid_t kp_pgid;
	pid_t kp_ppid;

	uid_t kp_uid;
	uid_t kp_ruid;
	uid_t kp_svuid;
	gid_t kp_gid;
	gid_t kp_rgid;
	gid_t kp_svgid;

	size_t kp_vsz;
	int kp_pri;
	int kp_nice;
	int kp_state;
	unsigned kp_pcpu;
	clock_t kp_ctime; /* cumulative cpu time */
	time_t kp_stime; /* start time */
	dev_t kp_tty;
#endif
	kp->kp_pid = pp->p_pid;
	kp->kp_pgid = pp->p_pgrp;
	kp->kp_ppid = pp->p_pptr ? pp->p_pptr->p_pid : 0;
	kp->kp_euid = pp->p_euid;
	kp->kp_ruid = pp->p_ruid;
	kp->kp_svuid = pp->p_svuid;
	kp->kp_egid = pp->p_egid;
	kp->kp_rgid = pp->p_rgid;
	kp->kp_svgid = pp->p_svgid;
	kp->kp_vsz = 0L; /* XXX */
	kp->kp_pri = 0;
	kp->kp_nice = pp->p_nice - NZERO;
	kp->kp_state = pp->p_state;
	kp->kp_pcpu = pp->p_pctcpu;
	kp->kp_ctime = pp->p_kru.kru_utime + pp->p_kru.kru_stime;
	kp->kp_cctime = pp->p_ckru.kru_utime + pp->p_ckru.kru_stime;
	kp->kp_stime = 0L; /* XXX */
	kp->kp_tty = pp->p_ttydev;
	strncpy(kp->kp_cmd, pp->p_name, MAXCMDLEN);
	kp->kp_cmd[MAXCMDLEN] = '\0';
	switch (pp->p_state) {
	case P_RUNNING:
		kp->kp_state = PRUN;
		break;
	case P_SLEEPING:
		if (pp->p_flag & P_SINTR)
			kp->kp_state = PSLEEP;
		else
			kp->kp_state = PDSLEEP;
		break;
	case P_STOPPED:
		kp->kp_state = PSTOPPED;
		break;
	case P_ZOMBIE:
		kp->kp_state = PZOMBIE;
		break;
	default:
		/* ??? */
		;
	}
}

typedef int proc_select(struct proc *, int);

static int select_all(struct proc *p, int to_be){ return to_be || !to_be; }
static int select_pid(struct proc *p, int pid)  { return (p->p_pid == pid); }
static int select_pgrp(struct proc *p, int pgrp){ return (p->p_pgrp == pgrp); }
static int select_tty(struct proc *p, int tty)  { return (p->p_ttydev == tty); }
static int select_uid(struct proc *p, int uid)  { return (p->p_euid == uid); }
static int select_ruid(struct proc *p, int ruid){ return (p->p_ruid == ruid); }

static int do_kern_proc(proc_select *selector, int arg,
                        void *oldp, size_t *oldlenp)
{
	struct proc *pp;
	struct kinfo_proc k;
	size_t len = 0;
	/*
	 * Loop through proc_list once to calculate the size of memory needed
	 * to hold all of the kinfo_proc structures at *oldp.
	 */
	list_for_each_entry(pp, &G.proc_list, p_list) {
		if (selector(pp, arg))
			++len;
	}
	len *= sizeof(struct kinfo_proc);
	if (oldp) {
		struct kinfo_proc *kp = oldp;
		if (*oldlenp < len) return ENOMEM;
		/*
		 * Loop through proc_list a second time, this time converting
		 * each selected proc structure to kproc_info and copying it
		 * out to *oldp.
		 */
		list_for_each_entry(pp, &G.proc_list, p_list) {
			if (!selector(pp, arg)) continue;
			proc_to_kinfo_proc(pp, &k);
			if (copyout(kp, &k, sizeof(struct kinfo_proc)))
				return EFAULT;
			++kp;
		}
	}
	*oldlenp = len;
	return 0;
}

static int kern_proc_all(int *name, unsigned namelen,
                         void *oldp, size_t *oldlenp)
{
	if (namelen < 0) return ENOTDIR;
	if (namelen > 0) return ENOENT;
	return do_kern_proc(select_all, 0, oldp, oldlenp);
}

static int kern_proc_pid(int *name, unsigned namelen,
                         void *oldp, size_t *oldlenp)
{
	if (namelen < 1) return ENOTDIR;
	if (namelen > 1) return ENOENT;
	return do_kern_proc(select_pid, name[1], oldp, oldlenp);
}

static int kern_proc_pgrp(int *name, unsigned namelen,
                          void *oldp, size_t *oldlenp)
{
	if (namelen < 1) return ENOTDIR;
	if (namelen > 1) return ENOENT;
	return do_kern_proc(select_pgrp, name[1], oldp, oldlenp);
}

static int kern_proc_tty(int *name, unsigned namelen,
                         void *oldp, size_t *oldlenp)
{
	if (namelen < 1) return ENOTDIR;
	if (namelen > 1) return ENOENT;
	return do_kern_proc(select_tty, name[1], oldp, oldlenp);
}

static int kern_proc_uid(int *name, unsigned namelen,
                         void *oldp, size_t *oldlenp)
{
	if (namelen < 1) return ENOTDIR;
	if (namelen > 1) return ENOENT;
	return do_kern_proc(select_uid, name[1], oldp, oldlenp);
}

static int kern_proc_ruid(int *name, unsigned namelen,
                          void *oldp, size_t *oldlenp)
{
	if (namelen < 1) return ENOTDIR;
	if (namelen > 1) return ENOENT;
	return do_kern_proc(select_ruid, name[1], oldp, oldlenp);
}

static int kern_uptime(int *name, unsigned namelen,
                       void *oldp, size_t *oldlenp)
{
	if (namelen < 0) return ENOTDIR;
	if (namelen > 0) return ENOENT;
	return putvalue(oldp, oldlenp, &uptime.tv_sec, sizeof(long));
}

/* return load averages array in 16.16 fixed-point format */
static int vm_loadavg(int *name, unsigned namelen, void *oldp, size_t *oldlenp)
{
	long load[3];
	int i;

	if (namelen < 0) return ENOTDIR;
	if (namelen > 0) return ENOENT;

	for (i = 0; i < 3; ++i)
		load[i] = G.loadavg[i] << (16 - F_SHIFT);

	return putvalue(oldp, oldlenp, load, sizeof(load));
}

static int vm_total(int *name, unsigned namelen, void *oldp, size_t *oldlenp)
{
	return ENOENT;
}

static const struct sysctl_table kern_proc = {
	6,
	{
	{ "all", CTL_FUNCTION, { .function=kern_proc_all } },
	{ "pid", CTL_FUNCTION, { .function=kern_proc_pid } },
	{ "pgrp", CTL_FUNCTION, { .function=kern_proc_pgrp } },
	{ "tty", CTL_FUNCTION, { .function=kern_proc_tty } },
	{ "uid", CTL_FUNCTION, { .function=kern_proc_uid } },
	{ "ruid", CTL_FUNCTION, { .function=kern_proc_ruid } },
	}
};

static const struct sysctl_table ctl_hw = {
	10,
	{
	{ "machine", CTL_STRING, { .string="" } },
	{ "model", CTL_STRING, { .string="" } },
	{ "ncpu", CTL_INTEGER, { .integer=1 } },
	{ "byteorder", CTL_INTEGER, { .integer=4321 } },
	{ "physmem", CTL_INTEGER, { .integer=256*1024L } },
	{ "usermem", CTL_INTEGER, { .integer=0 } },
	{ "pagesize", CTL_INTEGER, { .integer=0 } },
	{ "floatingpt", CTL_INTEGER, { .integer=0 } },
	{ "machine_arch", CTL_STRING, { .string="" } },
	{ "realmem", CTL_INTEGER, { .integer=256*1024L } },
	}
};

static const struct sysctl_table ctl_kern = {
	29,
	{
	{ "argmax", CTL_INTEGER, { .integer = ARG_MAX } },
	{ "bootfile", CTL_STRING, { .string = "" } },
	{ "boottime", CTL_UNSUPPORTED, { NULL } },
	{ "clockrate", CTL_UNSUPPORTED, { NULL } },
	{ "file", CTL_UNSUPPORTED, { NULL } },
	{ "hostid", CTL_UNSUPPORTED, { NULL } },
	{ "hostuuid", CTL_UNSUPPORTED, { NULL } },
	{ "hostname", CTL_UNSUPPORTED, { NULL } },
	{ "job_control", CTL_INTEGER, { .integer=1 } },
	{ "maxfiles", CTL_INTEGER, { .integer=LONG_MAX } },
	{ "maxfilesperproc", CTL_INTEGER, { .integer=NOFILE } },
	{ "maxproc", CTL_INTEGER, { .integer=LONG_MAX } },
	{ "maxprocperuid", CTL_UNSUPPORTED, { NULL } },
	{ "maxvnodes", CTL_UNSUPPORTED, { NULL } },
	{ "ngroups", CTL_INTEGER, { .integer=NGROUPS } },
	{ "nisdomainname", CTL_UNSUPPORTED, { NULL } },
	{ "osreldate", CTL_INTEGER, { .integer=201103L /* XXX */} },
	{ "osrelease", CTL_UNSUPPORTED, { NULL } },
	{ "osrev", CTL_UNSUPPORTED, { NULL } },
	{ "ostype", CTL_UNSUPPORTED, { NULL } },
	{ "posix1", CTL_INTEGER, { .integer=2001 /* ? */ } },
	{ "proc", CTL_TABLE, { .table=&kern_proc } },
	{ "prof", CTL_UNSUPPORTED, { NULL } },
	{ "quantum", CTL_INTEGER, { .integer=0 /* XXX */ } },
	{ "saved_ids", CTL_INTEGER, { .integer=1 } },
	{ "securelvl", CTL_INTEGER, { .integer=0 } },
	{ "updateinterval", CTL_INTEGER, { .integer=0 } },
	{ "uptime", CTL_INTEGERP, { .integerp=&uptime.tv_sec } },
	{ "version", CTL_INTEGER, { .integer=0 } },
	{ "vnode", CTL_UNSUPPORTED, { NULL } },
	}
};

static const struct sysctl_table ctl_vm = {
	11,
	{
	{ "loadavg", CTL_FUNCTION, { .function=vm_loadavg } },
	{ "total", CTL_FUNCTION, { .function=vm_total } },
	{ "pageout_algorithm", CTL_INTEGER, { .integer=0 } },
	{ "swapping_enabled", CTL_INTEGER, { .integer=0 } },
	{ "v_cache_max", CTL_INTEGER, { .integer=0 } },
	{ "v_cache_min", CTL_INTEGER, { .integer=0 } },
	{ "v_free_min", CTL_INTEGER, { .integer=0 } },
	{ "v_free_reserved", CTL_INTEGER, { .integer=0 } },
	{ "v_free_target", CTL_INTEGER, { .integer=0 } },
	{ "v_inactive_target", CTL_INTEGER, { .integer=0 } },
	{ "v_pageout_free_min", CTL_INTEGER, { .integer=0 } },
	}
};

static const struct sysctl_table ctl_root = {
	8,
	{
	{ "hw", CTL_TABLE, { .table=&ctl_hw } },
	{ "kern", CTL_TABLE, { .table=&ctl_kern } },
	{ "vm", CTL_TABLE, { .table=&ctl_vm } },
	{ "machdep", CTL_UNSUPPORTED, { NULL } },
	{ "vfs", CTL_UNSUPPORTED, { NULL } },
	{ "user", CTL_UNSUPPORTED, { NULL } },
	{ "net", CTL_UNSUPPORTED, { NULL } },
	{ "debug", CTL_UNSUPPORTED, { NULL } },
	}
};


static int getvalue(const struct sysctl_table *tp, int *name, unsigned namelen,
             void *oldp, size_t *oldlenp)
{
	struct sysctl_entry *ep;
	void *addr = NULL;
	size_t len = 0;
	if (namelen < 1) return ENOTDIR;
	if (name[0] < 0 || tp->size <= name[0]) return EINVAL;
	if (((long)oldlenp) & 1) return EFAULT; /* odd address */

	ep = &tp->entries[name[0]];
	
	switch (ep->type) {
	case CTL_UNSUPPORTED:
		return ENOENT;
	case CTL_TABLE:
		if (namelen < 2) return ENOTDIR;
		return getvalue(ep->get.table, name + 1, namelen - 1,
		                oldp, oldlenp);
	case CTL_FUNCTION:
		return ep->get.function(name + 1, namelen - 1, oldp, oldlenp);
	case CTL_INTEGER:
		len = sizeof(long);
		addr = &ep->get.integer;
		break;
	case CTL_INTEGERP:
		len = sizeof(long);
		addr = ep->get.integerp;
		break;
	case CTL_STRING:
		len = strlen(ep->get.string) + 1;
		addr = ep->get.string;
		break;
	}
	/* immediate values must be the last int in the name array */
	if (namelen > 1) return ENOENT;
	if (oldp) {
		if (*oldlenp < len)
			return ENOMEM;
		if (copyout(oldp, addr, len))
			return EFAULT;
	}
	*oldlenp = len;
	return 0;
}

extern int suser();

static int setvalue(const struct sysctl_table *tp, int *name, unsigned namelen,
                    void *newp, size_t newlen)
{
	struct sysctl_entry *ep;
	if (namelen < 1) return ENOTDIR;
	if (name[0] < 0 || tp->size <= name[0]) return EINVAL;
	//if (((long)newp) & 1) return EFAULT; /* odd address */
	
	ep = &tp->entries[name[0]];
	
	if (!ep->set.function || !suser()) return EPERM;
	
	switch (ep->type) {
	case CTL_UNSUPPORTED:
		return ENOENT;
	case CTL_TABLE:
		if (namelen < 2) return ENOTDIR;
		return setvalue(ep->set.table, name + 1, namelen - 1,
		                newp, newlen);
	case CTL_INTEGER:
		return EPERM;
	default:
		return ep->set.function(name + 1, namelen - 1, newp, newlen);
	}
	return 0;
}

struct sysctla {
	int *name;
	unsigned namelen;

	void *oldp;
	size_t *oldlenp;

	void *newp;
	size_t newlen;
};

void sys_sysctl()
{
	struct sysctla *ap = P.p_arg;
	int e;

	if (ap->namelen < 2 || CTL_MAXNAME < ap->namelen) {
		P.p_error = EINVAL;
		return;
	}

	e = getvalue(&ctl_root, ap->name, ap->namelen, ap->oldp, ap->oldlenp);
	if (e) P.p_error = e;
	if (!ap->newp)
		return;
	e = setvalue(&ctl_root, ap->name, ap->namelen, ap->newp, ap->newlen);
	if (e) P.p_error = e;
}

/* FIXME: this probably doesn't handle sizep correctly */
static int nametomib(const struct sysctl_table *tp, const char *name,
                     int *mibp, size_t *sizep)
{
	int len;
	int i;
	int v = -1;
	int err = 0;
	struct sysctl_entry *ep;
	const char *dot;
	if (mibp && *sizep == 0) return ENOMEM;
	dot = strchr(name, '.');
	if (dot)
		len = dot - name;
	else
		len = strlen(name);
	for (ep = &tp->entries[0]; ep < &tp->entries[tp->size]; ++ep) {
		if (strlen(ep->name) == len && !strncmp(ep->name, name, len)) {
			v = ep - &tp->entries[0];
			if (name[len] != '\0') {
				if (ep->type == CTL_TABLE) {
					--*sizep;
					err = nametomib(ep->get.table,
					  name+len+1, mibp?mibp+1:0, sizep);
					if (err) return err;
					++*sizep;
				} else {
					return ENOENT;
				}
			} else {
				*sizep = 1;
			}
			if (mibp)
				*mibp = v;
			break;
		}
	}
	return ENOENT;
}

/* return the count of the character 'c' in the string 's' */
size_t strnchr(const char *s, int c)
{
	size_t n = 0;
	const char *p;
	while ((p = strchr(s, c))) {
		++n;
		s = p+1;
	}
	return n;
}

/*
 * this can actually be a user-level routine using a trimmed-down sysctl_table
 * structure
 */
int sysctlnametomib(const char *name, int *mibp, size_t *sizep)
{
	if (strnchr(name, '.') >= CTL_MAXNAME)
		return ENOENT;
	
	return nametomib(&ctl_root, name, mibp, sizep);
}
