#include <termios.h>

#include "flash.h"
#include "callout.h"
#include "queue.h"
#include "inode.h"
#include "tty.h"
#include "heap.h"
#include "buf.h"
#include "kbd.h"
#include "glyph.h"

struct globals {
	struct timespec _walltime; /* must be first! see entry.s (Int_3) */
	struct timespec _realtime;
	long _timedelta;
	char exec_ram[60];
	int _runrun;
	int _istick;
	int _ioport;
	int _cputime;
	int _updlock;
	struct proc *_current;
	int lowestpri;
	/* struct proc *freeproc; */
	struct proc *initproc;
	struct proc *proclist;
	int numrunning;
	long cumulrunning;
	struct file file[NFILE];
	unsigned long loadavg[3];
	struct timespec _uptime;
	long _loadavtime;
	
	int audiosamp; /* current samples */
	int audiosamples; /* number of samples within that byte */
	int audiolowat; /* low water level in audio queue */
	int audioplay; /* flag to indicate if audio should play */
	long long audiooptr;
	
	int linklowat;
	
	struct queue linkreadq, linkwriteq, audioq;
	
	/* seed for the pseudo-random number generator */
	unsigned long prngseed;
	
	dev_t rootdev, pipedev;
	struct inode *rootdir;
	
	char canonb[CANBSIZ];
	struct inode inode[NINODE];
	struct inode *inodelist;
	uid_t mpid;
	unsigned int pidchecked;
	struct callout callout[NCALL];
	
	struct buf avbuflist; /* list of buf */
	int numbufs;
	/* struct buf buf[NBUF]; */
	
	struct flashblock *currentfblock;
	struct flash_cache_entry flash_cache[FLASH_CACHE_SIZE];
	
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
		struct glyphset *glyphset, *charsets[2];
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
		short key;
		short key_compose;
		unsigned char key_caps;
		unsigned char compose;
		unsigned char key_repeat; /* repeat enabled? */
		unsigned char key_repeat_delay;
		unsigned char key_repeat_start_delay;
		unsigned char key_repeat_counter;
		short key_previous;
		char gr; /* graphics rendition */
		int lock;
	} vt;
	
	int batt_level;
	
	/* temp/debugging variables */
	int whereami;
	int spin;
	struct timeval lasttime;
	struct rusage lastrusage;
	char charbuf[128];
	int charbufsize;
	int nextinode;
	/* end temp/debugging variables */
	
	int heapsize;
	struct heapentry heaplist[HEAPSIZE];
	char heap[0][HEAPBLOCKSIZE];
};

# if 0
extern struct globals G;
extern int runrun, ioport;
extern long walltime;

/* cputime is the length of time the current process has been running
 * since it's been switched in, measured in half-ticks (15.1 fixed-point) */
extern int cputime;
extern int updlock;
# else

#define G (*(struct globals *)0x5c00)
#define walltime G._walltime
#define realtime G._realtime
#define timedelta  G._timedelta
#define runrun   G._runrun
#define istick   G._istick
#define ioport   G._ioport
#define cputime  G._cputime
#define updlock  G._updlock
#define current  G._current
#define loadavtime G._loadavtime
#define uptime   G._uptime

# endif
