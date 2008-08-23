#include <termios.h>

#include "tty.h"
#include "callout.h"

struct globals {
	char exec_ram[60];
	struct timespec _walltime;
	int _runrun;
	int _istick;
	int _ioport;
	int _cputime;
	int _updlock;
	int lowestpri;
	
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
	
	int cellrow;
	int cellcol;
	/* char cells[20][60]; */
	
	/* the following is for testing execve(). */
#define USTACKSIZE 1024
	char ustack[USTACKSIZE];
	
	char canonb[CANBSIZ];
	struct inode inode[NINODE];
	struct proc proc[NPROC];
	struct proc *freeproc;
	struct proc *current;
	struct file *file[NFILE];
	uid_t mpid;
	unsigned int pidchecked;
	struct callout callout[NCALL];
	
	/* dev_vt static variables */
	int xon;
	int privflag;
	int nullop;
	char intchars[2+1];
	char *intcharp;
	unsigned params[16];
	int numparams;
	int cursorvisible;
	int tabstops[60];
	struct state *vtstate;
	struct glyphset *glyphset, *charsets[2];
	int charset;
	struct pos {
		int row, column;
	} pos;
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
	struct tty vt[1];
	
	int heap[0];
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
#define runrun   G._runrun
#define istick   G._istick
#define ioport   G._ioport
#define cputime  G._cputime
#define updlock  G._updlock

# endif
