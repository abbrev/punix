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

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define TRACE2(f, l) do { *(long *)(0x4c00+0xf00-22) = f " (" STRING(l) ")"; } while (0)
#define TRACE() TRACE2(__FILE__, __LINE__)

/*
 * masklock is used for soft interrupts (see trap.c).
 * Critical sections which must not be interrupted by a soft interrupt, but
 * can be interrupted by hard interrupts, should use a masklock. The typical
 * usage of a masklock in a soft interrupt is like this:
 *
 * if (mask(&lock)) {
 *   // ...
 * }
 * unmask(&lock);
 *
 * In a critical section it can be used like this:
 * mask(&lock);
 * // critical section
 * unmask(&lock);
 *
 * mask() can be nested up to 255 times (which should be more than enough).
 */
typedef int masklock;
int mask(masklock *);
void unmask(masklock *);

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

static inline void nop() { asm volatile ("nop"); }
static inline void halt() { asm volatile ("stop #0x2700"); }
static inline void splx(int x)
{
	asm volatile ("move %0,%%d0; move %%d0,%%sr"
	              : /* no output */
	              : "gr"(x)
	              : "d0");
}
static inline int spl(int x)
{
	int ret;
	asm volatile ("move %%sr,%%d0; move %%d0,%0 ; move %1*256+0x2000,%%sr"
	              : "=r"(ret)
	              : "i"(x)
	              : "d0");
	return ret;
}
#define spl0() spl(0)
#define spl1() spl(1)
#define spl2() spl(2)
#define spl3() spl(3)
#define spl4() spl(4)
#define spl5() spl(5)
#define spl6() spl(6)
#define spl7() spl(7)
#define splclock() spl1()

void stop(struct proc *);

void panic(const char *s);
void warn(const char *s, long value);

int copyin(void *dest, const void *src, size_t count);
int copyout(void *dest, const void *src, size_t count);
int passc(int ch);
int cpass();

int kprintf(const char *, ...);

int inferior(struct proc *);
struct proc *pfind(int pid);

#endif /* _SYS_PUNIX_H_ */
