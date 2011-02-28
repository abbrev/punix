#ifndef _SYS_PUNIX_H_
#define _SYS_PUNIX_H_

/* $Id$ */

#include <sys/types.h>
#include "list.h"
#include "proc.h"

#if 0
#define OS_NAME	"Punix"
#define OS_VERSION	"0.06"
#endif

extern const char
uname_sysname[],
uname_nodename[],
uname_release[],
uname_version[],
uname_machine[];

#define STARTUP(x) \
	x __attribute__ ((section ("_st1"))); \
	x

#define INTHANDLER(x) \
	x __attribute__ ((section ("_st1") interrupt_handler)); \
	x

struct devmm_t {
	unsigned char d_major;
	unsigned char d_minor;
};

#define MAJOR(d) (((struct devmm_t *)&(d))->d_major)
#define MINOR(d) (((struct devmm_t *)&(d))->d_minor)

struct syscallframe {
	void *regs[10];
	unsigned short sr;
	void *pc;
};

#ifndef NULL
#define NULL (void *)0
#endif

#define NODEV (dev_t)(-1)

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define BLOCKSHIFT 7
#define BLOCKSIZE (1<<BLOCKSHIFT)
#define BLOCKMASK (~BLOCKSIZE)

#define RORB(n, b) (((n) >> (b)) | ((n) << (8 - (b))))
#define RORW(n, b) (((n) >> (b)) | ((n) << (16 - (b))))
#define RORL(n, b) (((n) >> (b)) | ((n) << (32 - (b))))
#define ROR RORW
#define ROLB(n, b) (((n) << (b)) | ((n) >> (8 - (b))))
#define ROLW(n, b) (((n) << (b)) | ((n) >> (16 - (b))))
#define ROLL(n, b) (((n) << (b)) | ((n) >> (32 - (b))))
#define ROL ROLW

/* useful macro for iterating through each process */
//#define EACHPROC(p)	((p) = G.proclist; (p); (p) = (p)->p_next)

int spl0(void), spl1(void), spl2(void), spl3(void);
int spl4(void), spl5(void), spl6(void), spl7(void);
int splx(int);
int splclock(void);
void stop(struct proc *);

void panic(const char *s);
void warn(const char *s, long value);

int copyin(void *dest, const void *src, size_t count);
int copyout(void *dest, const void *src, size_t count);
int passc(int ch);
int cpass();

void kprintf(const char *, ...);

int inferior(struct proc *);
struct proc *pfind(int pid);

#endif /* _SYS_PUNIX_H_ */
