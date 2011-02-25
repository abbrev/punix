#ifndef _PARAM_H_
#define _PARAM_H_

/* debugging constants */
#define WHEREAMI_USER         0
#define WHEREAMI_GETLOADAVG1  1
#define WHEREAMI_SYSCALL      2
#define WHEREAMI_WRITE        3
#define WHEREAMI_GETTIMEOFDAY 4
#define WHEREAMI_VTWRITE      15
#define WHEREAMI_PRINT        16

/*
 * tunable variables
 */

#define	NFILE	175		/* number of in core file structures (XXX) */
#define	NINODE	200		/* number of in core inodes (XXX) */
#define	NICINOD	100		/* number of superblock inodes (XXX) */
#define	NICFREE	50		/* number of superblock free blocks (XXX) */

#define	MINBUF	8		/* minimum number of buffers */
#define MAXBUF  32
#define	NMOUNT	8		/* number of mountable file systems */
#define	MAXMEM	(64*32)		/* max core per process - first # is Kw */
#define	MAXUPRC	25		/* max processes per user */
#define	NOFILE	32		/* max open files per process */
#define	CANBSIZ	QSIZE		/* max size of typewriter line */
#define	NCALL	20		/* max simultaneous time callouts */
#define	NGROUPS	16		/* max number of groups */
#define	NOGROUP	((gid_t)-1)		
#define	NTEXT	40		/* max number of pure texts */
#define	HZ	256		/* clock rate (ticks/second) */
#define	SECOND  1000000000L	/* nanoseconds in a second */
#define TICK	(SECOND / HZ) /* nanoseconds/tick of the clock */

#define FLASH_CACHE_SIZE 32
#define HEAPSIZE 512
#define HEAPBLOCKSIZE 16

#define	TIMEZONE (7*60)		/* Minutes westward from Greenwich */
#define	DSTFLAG	0		/* Daylight Saving Time applies in this locality */

#define MAXSYMLINKS 6
#define MAXPATHLEN 256

/*
 * Fixed-point values for load averages (see loadav.c).
 *
 * The value of F_SHIFT affects the range of values. The maximum value is
 * 2^(32-2*F_SHIFT) - 1. For example, when F_SHIFT is 13, the maximum is
 * 2^(32-2*13) - 1 = 2^6 - 1 = 63. On this system (TI-92+ with 256K RAM), a load
 * average limit of 63 is probably acceptable, since at this maximum load, each
 * process would have to be only 3K stack + data + text (about 1K is for the
 * proc structure itself) on average.
 */
#define F_SHIFT 13
#define F_ONE (1 << F_SHIFT)
#define F_HALF (1 << (F_SHIFT - 1))
#define EXP_1  ((long)(0.920044415 * F_ONE + 0.5)) /* exp(-5/60) */
#define EXP_5  ((long)(0.983471454 * F_ONE + 0.5)) /* exp(-5/300) */
#define EXP_15 ((long)(0.994459848 * F_ONE + 0.5)) /* exp(-5/900) */

#define KEY_REPEAT_DELAY	500 /* milliseconds to delay before repeating */
#define KEY_REPEAT_RATE		32  /* repeats per second */

/*
 * priorities
 * probably should not be
 * altered too much
 */

#define	PSWP	0
#define	PINOD	10
#define PAUDIO	10 /* XXX */
#define	PRIBIO	20
#define	PZERO	25
#define	NZERO	20
#define	PPIPE	26
#define	PWAIT	30
#define	PSLEP	40
#define	PUSER	50

#define PRIMASK 0xff
#define PCATCH  0x100

/*
 * fundamental constants of the implementation--
 * cannot be changed easily
 */

#define	NBPW	sizeof(int)	/* number of bytes in an integer */
#define	BSIZE	512		/* size of secondary block (bytes) */
#define	BMASK	0777		/* BSIZE-1 */
#define	BSHIFT	9		/* LOG2(BSIZE) */
#define	USIZE	16		/* size of user block (*64) */
#define	CMASK	0		/* default mask for file creation */
#define	NODEV	(dev_t)(-1)
//#define	ROOTINO	((ino_t)2)	/* i number of all roots */
//#define	SUPERB	1		/* block number of the super block */
//#define	NICINOD	100		/* number of superblock inodes */
//#define	NICFREE	50		/* number of superblock free blocks */
//#define	INFSIZE	138		/* size of per-proc info for users */

#if 0 /* make this work in Punix on m68k */
/*
 * Some macros for units conversion
 */
/* Core clicks (64 bytes) to segments and vice versa */
#define	ctos(x)	((x+127)/128)
#define stoc(x) ((x)*128)

/* Core clicks (64 bytes) to disk blocks */
#define	ctod(x)	((x+7)>>3)

/* inumber to disk address */
#define	itod(x)	(daddr_t)((((unsigned)x+15)>>3))

/* inumber to disk offset */
#define	itoo(x)	(int)((x+15)&07)

/* clicks to bytes */
#define	ctob(x)	(x<<6)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)x+63)>>6))

/* major part of a device */
#define	major(x)	(int)(((unsigned)x>>8))

/* minor part of a device */
#define	minor(x)	(int)(x&0377)

/* make a device number */
#define	makedev(x,y)	(dev_t)((x)<<8 | (y))

typedef	long		daddr_t;
typedef char *		caddr_t;
#endif

#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))

#define itod(x) (x<<16)
#define itoo(x) 0

/*
 * Machine-dependent bits and macros
 */
#define	UMODE	0x2000		/* usermode bits */
#define	USERMODE(ps)	(((ps) & UMODE)==0)

#define DEV_BSIZE  128
#define DEV_BSHIFT 7
#define DEV_BMASK  0x7f

#endif
