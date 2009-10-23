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
}

void ttychars(struct tty *tp)
{
        tp->t_cc[VINTR] = CINTR;
        tp->t_cc[VQUIT] = CQUIT;
        tp->t_cc[VSTART] = CSTART;
        tp->t_cc[VSTOP] = CSTOP;
        tp->t_cc[VEOF] = CEOT;
        tp->t_cc[VEOL] = CEOL;
        /*tp->t_cc[VBRK] = CBRK;*/
        tp->t_cc[VERASE] = CERASE;
	tp->t_cc[VWERASE] = CWERASE;
        tp->t_cc[VKILL] = CKILL;
}

void flushtty(struct tty *tp)
{
	/* FIXME */
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

void ttyread(struct tty *tp)
{
	int ch;
	int lflag = tp->t_lflag;
	cc_t *cc = tp->t_termios.c_cc;
	struct queue *qp;
	int havec = 0;
	
	qp = (lflag & ICANON) ? &tp->t_canq : &tp->t_rawq;
	
loop:
	/* TODO: also check for the tty closing */
	while (qisempty(qp)) {
		slp(&tp->t_rawq, TTIPRI | PCATCH);
	}
	
	while ((ch = getc(qp)) >= 0) {
		if ((lflag & ICANON)) {
			int numc = tp->t_numc;
			tp->t_numc = !TTBREAKC(ch);
			if (ch == cc[VEOF]) {
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
	wakeup((void *)&tp->t_rawq);
}


/* Following functions should be moved into some other file */

/* copy buffer to queue. return number of bytes not copied */
/* TODO: optimize this by copying blocks of bytes from the buffer to the queue
 * rather than copying one byte at a time */
int b_to_q(char *bp, int count, struct queue *qp)
{
	int n = count;
	
	while (n && putc(*bp++, qp) >= 0)
		--n;
	
	return n;
}

/* copy queue to buffer. return number of bytes copied */
/* TODO: optimize this by copying blocks of bytes from the queue to the buffer
 * rather than copying one byte at a time */
int q_to_b(struct queue *qp, char *bp, int count)
{
	int n = 0;
	int ch;
	
	while (n < count && (ch = getc(qp)) >= 0) {
		*bp++ = ch;
		++n;
	}
	
	return n;
}

/* concatenate from queue 'from' to queue 'to' */
/* TODO: this can also be optimized much as b_to_q and q_to_b can */
void catq(struct queue *from, struct queue *to)
{
	int ch;
	while ((ch = getc(from)) >= 0) {
		if (putc(ch, to) < 0) {
			ungetc(ch, from);
			break;
		}
	}
}
