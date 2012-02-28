#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include "punix.h"
#include "buf.h"
#include "dev.h"
#include "queue.h"
#include "proc.h"
#include "inode.h"
#include "globals.h"
#include "tty.h"

void ttyopen(dev_t dev, struct tty *tp);
void ttychars(struct tty *tp);
void flushtty(struct tty *tp);
void wflushtty(struct tty *tp);
void ttyclose(struct tty *tp);
void ttyioctl(dev_t dev, int cmd, void *cmarg);
// static int canon(struct tty *tp);
void ttyread(struct tty *tp);
void ttyinput(int ch, struct tty *tp);
void ttyoutput(int ch, struct tty *tp);


void ttyopen(dev_t dev, struct tty *tp)
{
	tp->t_dev = dev;
	tp->t_state |= ISOPEN;
	tp->t_numc = 0; /* number of characters after a break character */
	/* if we're not already part of a process group, make this tty our
	 * controlling tty and add us to the tty's process group */
	if (P.p_pgrp == 0) {
		P.p_ttyp = tp;
		P.p_ttydev = dev;
		
		if (tp->t_pgrp == 0)
			tp->t_pgrp = P.p_pid;
		P.p_pgrp = tp->t_pgrp;
	}
	qinit((queue *)&tp->t_rawq, LOG2TTYQSIZE);
	qinit((queue *)&tp->t_canq, LOG2TTYQSIZE);
	qinit((queue *)&tp->t_outq, LOG2TTYQSIZE);
}

void ttychars(struct tty *tp)
{
	tp->t_cc[VINTR] = CINTR;
	tp->t_cc[VQUIT] = CQUIT;
	tp->t_cc[VSUSP] = CSUSP;
	tp->t_cc[VSTART] = CSTART;
	tp->t_cc[VSTOP] = CSTOP;
	tp->t_cc[VEOF] = CEOT;
	tp->t_cc[VEOL] = CEOL;
	/*tp->t_cc[VBRK] = CBRK;*/
	tp->t_cc[VERASE] = CERASE;
	tp->t_cc[VWERASE] = CWERASE;
	tp->t_cc[VKILL] = CKILL;
	tp->t_cc[VREPRINT] = CREPRINT;
}

void flushtty(struct tty *tp)
{
	/* FIXME */
	qclear((queue *)&tp->t_rawq);
	qclear((queue *)&tp->t_canq);
}

void wflushtty(struct tty *tp)
{
	/* FIXME */
}

void ttyclose(struct tty *tp)
{
	wflushtty(tp);
	tp->t_pgrp = 0;
	tp->t_state = 0;
}

void ttyioctl(dev_t dev, int cmd, void *cmarg)
{
	/* FIXME */
}

/*
 * FIXME: make this behave like the tty in Linux in canonical mode.
 * The tty in Linux buffers up to QSIZE - 1 characters in a line to leave room
 * for a newline or end-of-file (^D) character, either of which will send the
 * current line to the process.
 */
void ttyread(struct tty *tp)
{
	int ch;
	int lflag = tp->t_lflag;
	cc_t *cc = tp->t_termios.c_cc;
	queue *qp;
	int havec = 0;
	
	qp = (lflag & ICANON) ? (queue *)&tp->t_canq : (queue *)&tp->t_rawq;
	
loop:
	/* TODO: also check for the tty closing */
	spl1();  // inhibit soft interrupts
	while (qisempty(qp)) {
		slp(&tp->t_rawq, 1);
	}
	spl0();
	
	while ((ch = qgetc(qp)) >= 0) {
		if ((lflag & ICANON)) {
			int numc = tp->t_numc;
			tp->t_numc = !TTBREAKC(ch);
			if (cc[VEOF] && ch == cc[VEOF]) {
				//kprintf("numc=%d havec=%d\n", numc, havec);
				if (numc && !havec) {
					//kprintf("goto loop\n");
					goto loop;
				} else {
					//kprintf("break\n");
					break; 
				}
			}
		}
		havec = 1;
		if (passc(ch) < 0)
			break;
		if ((lflag & ICANON) && TTBREAKC(ch))
			break;
	}
}

void ttyinput(int ch, struct tty *tp)
{
	/* FIXME */
}

void ttyoutput(int ch, struct tty *tp)
{
	/* FIXME */
}

/* ttywakeup is from 4.4BSD-Lite */
/*
 * Wake up any readers on a tty.
 */
void ttywakeup(struct tty *tp)
{

/*
	selwakeup(&tp->t_rsel);
	if (ISSET(tp->t_state, TS_ASYNC))
		pgsignal(tp->t_pgrp, SIGIO, 1);
*/
	TRACE();
	wakeup((void *)&tp->t_rawq);
}


/* Following functions should be moved into some other file */

/* copy buffer to queue. return number of bytes copied */
#if 1
int b_to_q(char const *bp, int count, queue *qp)
{
	int n = count;
	unsigned long head = qp->q_head & qp->q_mask;
	unsigned long tail = qp->q_tail & qp->q_mask;
	/* limit n to the free size of our queue */
	if (n > qfree(qp))
		n = qfree(qp);
	if (n == 0)
		return 0;
	
	if (head < tail || head + n <= qsize(qp)) {
		/* easy case: one contiguous block */
		memmove(qp->q_buf + head, bp, n);
	} else {
		/* split case: two separate blocks */
		int y = qsize(qp) - head; /* upper block byte count */
		int z = n - y; /* lower block byte count */
		memmove(qp->q_buf + head, bp, y); /* copy upper block */
		memmove(qp->q_buf, bp + y, z); /* copy lower block */
	}
	qp->q_head += n;
	
	return n;
}
#else
/* unoptimized version */
int b_to_q(char const *bp, int count, queue *qp)
{
	int n = count;
	
	while (n && qputc(*bp++, qp) >= 0)
		--n;
	
	return count - n;
}
#endif

/* copy queue to buffer. return number of bytes copied */
#if 1
/* XXX: this has NOT been tested */
int q_to_b(queue *qp, char *bp, int count)
{
	int n = count;
	unsigned long tail = qp->q_tail & qp->q_mask;
	unsigned long used = qused(qp);
	/* limit n to the used size of our queue */
	if (n > used)
		n = used;
	if (n == 0)
		return 0;
	
	if (tail < (qp->q_head & qp->q_mask) || tail + n <= qsize(qp)) {
		/* single block */
		memmove(bp, qp->q_buf + tail, n);
	} else {
		/* two blocks */
		int y = qsize(qp) - tail; /* upper block byte count */
		int z = n - y; /* lower block byte count */
		memmove(bp, qp->q_buf + tail, y); /* copy upper block */
		memmove(bp + y, qp->q_buf, z); /* copy lower block */
	}
	qp->q_tail += n;

	return n;
}
#else
/* unoptimized version */
int q_to_b(queue *qp, char *bp, int count)
{
	int n = 0;
	int ch;
	
	while (n < count && (ch = qgetc(qp)) >= 0) {
		*bp++ = ch;
		++n;
	}
	
	return n;
}
#endif

/* concatenate from queue 'from' to queue 'to' */
/* TODO: this can also be optimized much as b_to_q and q_to_b can */
void catq(queue *from, queue *to)
{
	int ch;
	while ((ch = qgetc(from)) >= 0) {
		if (qputc(ch, to) < 0) {
			qungetc(ch, (queue *)from);
			break;
		}
	}
}
