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

STARTUP(void ttyopen(dev_t dev, struct tty *tp))
{
	tp->t_dev = dev;
	if (P.p_pgrp == 0) {
		P.p_ttyp = tp;
		P.p_ttydev = dev;
		
		if (tp->t_pgrp == 0)
			tp->t_pgrp = P.p_pid;
		P.p_pgrp = tp->t_pgrp;
	}
	tp->t_state |= ISOPEN;
}

STARTUP(void ttychars(struct tty *tp))
{
	tp->t_termios.c_cc[VINTR] = CINTR;
	tp->t_termios.c_cc[VQUIT] = CQUIT;
	tp->t_termios.c_cc[VSTART] = CSTART;
	tp->t_termios.c_cc[VSTOP] = CSTOP;
	tp->t_termios.c_cc[VEOF] = CEOT;
	tp->t_termios.c_cc[VEOL] = CEOL;
	/*tp->t_termios.c_cc[VBRK] = CBRK;*/
	tp->t_termios.c_cc[VERASE] = CERASE;
	tp->t_termios.c_cc[VKILL] = CKILL;
}

STARTUP(void flushtty(struct tty *tp))
{
	int x;
	
	qclear(&tp->t_canq);
	wakeup(&tp->t_rawq);
	wakeup(&tp->t_outq);
	
	x = spl1();
	tp->t_state &= ~TTSTOP;
	qclear(&tp->t_outq);
	qclear(&tp->t_rawq);
	tp->t_delct = 0;
	splx(x);
}

STARTUP(void wflushtty(struct tty *tp))
{
	int x = spl1();
	while (!qisempty(&tp->t_outq) && tp->t_state & ISOPEN) {
		slp(&tp->t_outq, TTOPRI);
	}
	flushtty(tp);
	splx(x);
}

STARTUP(void ttyclose(struct tty *tp))
{
	tp->t_pgrp = 0;
	wflushtty(tp);
	tp->t_state = 0;
}

STARTUP(void ttyioctl(dev_t dev, int cmd, void *cmarg))
{
}

/* XXX: rewrite this! */
STARTUP(int canon(struct tty *tp))
{
	char *bp, *bp1;
	int ch;
	int mc;
	int iflag = tp->t_termios.c_iflag;
	int lflag = tp->t_termios.c_lflag;
	cc_t *cc = tp->t_termios.c_cc;
	
	int x = spl1();
	while (qisempty(&tp->t_rawq) || (lflag & ICANON && tp->t_delct == 0)) {
		if (!(tp->t_state & ISOPEN))
			return 0;
		slp(&tp->t_rawq, TTIPRI);
	}
	splx(x);
	
loop:
	bp = &G.canonb[2];
	while ((ch = getc(&tp->t_rawq)) >= 0) {
		if (ch == 0xff) {
			--tp->t_delct;
			break;
		}
		
		if (lflag & ICANON) {
/*
			if (ch == cc[VEOL] || ch == cc[VEOF])
				--tp->t_delct;
*/
			if (ch == cc[VERASE]) {
				if (bp > &G.canonb[2])
					--bp;
				continue;
			}
			if (ch == cc[VKILL])
				goto loop;
			if (ch == cc[VEOF])
				continue;
		}
		
		*bp++ = ch;
		if (bp >= &G.canonb[CANBSIZ])
			break;
	}
	bp1 = bp;
	bp = &G.canonb[2];
	while (bp < bp1)
		putc(*bp++, &tp->t_canq);
	
	return 1;
}

STARTUP(void ttyread(struct tty *tp))
{
	if (((tp->t_state & ISOPEN) && !qisempty(&tp->t_canq)) || canon(tp))
		while (!qisempty(&tp->t_canq) && passc(getc(&tp->t_canq)) >= 0)
			;
}

#if 0
STARTUP(void ttyinput(int ch, struct tty *tp))
{
	int iflag = tp->t_termios.t_iflag;
	int lflag = tp->t_termios.t_lflag;
	
	if ((ch &= 0x7f) == '\r' && iflag & ICRNL)
		ch = '\n';
	if ((lflag & ICANON) && (ch == tp->t_termios.c_cc[VQUIT] ||
	  ch == tp->t_termios.c_cc[VINTR])) {
		signal(tp, ch == tp->t_termios.c_cc[VQUIT] ? SIGINT : SIGQUIT);
		flushtty(tp);
		return;
	}
	if (qisfull(&tp->t_rawq)) {
		flushtty(tp);
		return;
	}
	putc(ch, &tp->t_rawq);
	if ((lflag & ICANON) == 0 || c == '\n' || c == 004) {
		wakeup(&tp->t_rawq);
		if (putc(0xff, &tp->t_rawq) >= 0)
			++tp->t_delct;
	}
	if (lflag & ECHO) {
		ttyoutput(ch, tp);
		ttstart(tp);
	}
}

STARTUP(void ttyoutput(int ch, struct tty *tp))
{
	/* FIXME: write this! */
}

#endif
