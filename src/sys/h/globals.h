#include <termios.h>
#include <setjmp.h>

#include "flash.h"
#include "callout.h"
#include "queue.h"
#include "inode.h"
#include "tty.h"
#include "heap.h"
#include "buf.h"
#include "kbd.h"
#include "glyph.h"
#include "sched.h"

struct globals {
	long seconds; /* XXX: see entry.s */
	struct timespec _realtime;
	
	/* all RAM below here can (should) be cleared on boot. see start.s */
	char exec_ram[60]; /* XXX: see flash.s */
	char fpram[8*12+3*4]; /* XXX: see fpuem.s */
	int onkey; /* set to 1 when ON key is pressed. see entry.s */
	int powerstate; /* set to 1 when power is off. see entry.s */
	/*
	 * realtime_mono monotonically increases and is never adjusted (only
	 * incremented). This is used with ITIMER_REAL timers and can be used
	 * with clock_gettime() when clk_id = CLOCK_MONOTONIC.
	 * Note: the absolute value of this clock is arbitrary. Only
	 * differences between values are meaningful.
	 */
	struct timespec _realtime_mono;
	long _timeadj;
	/* this is for getrealtime() */
	struct {
		time_t lasttime;
		long incr;
	} rt;
	
	int _ioport;
	int _updlock;
	struct proc *_current;
	int lowestpri;
	/* struct proc *freeproc; */
	struct proc *initproc;
	struct proc *proclist;
	struct list_head proc_list;
	int numrunning;
	long cumulrunning;
	struct file file[NFILE];
	unsigned long loadavg[3];
	/*
	 * Note: we *could* use realtime_mono as the uptime clock and get rid
	 * of the uptime variable. Both clocks count up the same way.
	 */
	struct timespec _uptime;
	long _loadavtime;
	
	//struct list_head runqueue;
	long prio_ratios[40];
	int need_resched;
	struct list_head runqueues[PRIO_LIMIT];
	volatile unsigned long ticks;

	struct {
		unsigned char samp; /* current samples */
		int samples; /* number of samples within that byte */
		int lowat; /* low water level in audio queue */
		int play; /* flag to indicate if audio should play */
		long long optr;
		
		/* we should probably use a more efficient
		 * structure than a queue for audio */
		struct queue q;
	} audio;
	
	struct {
		int lowat, hiwat;
		struct queue readq, writeq;
		char control;
		int readoverflow;
	} link;
	
	/* seed for the pseudo-random number generator */
	unsigned long prngseed;
	
	struct {
		//struct mount mounts[NMOUNT];
		// TODO: move all fs-related variables to here
	} fs;
	
	dev_t rootdev, pipedev;
	struct inode *rootdir;
	
	char canonb[CANBSIZ];
	struct inode inode[NINODE];
	//struct inode *inodelist;
	//struct list_head inode_list;
	uid_t mpid;
	unsigned int pidchecked;
	struct callout callout[NCALL];
	int calloutlock;
	
	struct buf avbuflist; /* list of buf */
	struct list_head avbuf_list; /* list of available buf */
	int numbufs;
	/* struct buf buf[NBUF]; */
	
	struct {
		struct flashblock *currentfblock;
		struct flash_cache_entry flash_cache[FLASH_CACHE_SIZE];
	} flash;
	
	int contrast;
	/* dev_vt static variables */
	struct {
		unsigned char xon;
		unsigned char nullop;
		int privflag;
		char intchars[2+1];
		char *intcharp;
		int params[16];
		unsigned char numparams;
		unsigned char cursorvisible;
		unsigned char tabstops[(60+7)/8];
		struct state const *vtstate;
		const struct glyphset *glyphset, *charsets[2];
		unsigned char charset;
		unsigned char margintop, marginbottom;
		struct pos {
			int row, column;
		} pos;
#if 0
		struct row {
			struct cell {
				struct attrib {
					int bold:1;
					int underscore:1;
					int blink:1;
					int reverse:1;
				} attrib;
				int c;
			} cells[60];
		} screen[20];
#endif
		struct tty vt[1];
		
		char key_array[KEY_NBR_ROW];
		short key_mod, key_mod_sticky;
		short key_compose;
		unsigned char caps_lock;
		union {
			unsigned char alpha_lock;
			unsigned char hand_lock;
		};
		unsigned char compose;
		unsigned char key_repeat; /* repeat enabled? */
		unsigned char key_repeat_delay;
		unsigned char key_repeat_start_delay;
		unsigned char key_repeat_counter;
		short key_previous;
		char gr; /* graphics rendition */
		int lock;
		int scroll_lock;
		int bell;
	} vt;
	int cpubusy;
	
	int batt_level;
	
	/* temp/debugging variables */
	int whereami;
	int spin;
	struct {
		char charbuf[128];
		int charbufsize;
		int _errno;
		jmp_buf getcalcjmp;
	} user;
	/* end temp/debugging variables */
	
	/* heap static variables (this must be last!) */
	struct {
		int heapsize;
		struct heapentry heaplist[HEAPSIZE];
		char heap[0][HEAPBLOCKSIZE];
	} heap;
};

# if 0
extern struct globals G;
extern int ioport;
extern long walltime;

extern int updlock;
# else

#define G (*(struct globals *)0x5c00)
#define realtime G._realtime
#define realtime_mono G._realtime_mono
#define timeadj  G._timeadj
#define ioport   G._ioport
#define updlock  G._updlock
#define current  G._current
#define loadavtime G._loadavtime
#define uptime   G._uptime

#define errno  G.user._errno

# endif
