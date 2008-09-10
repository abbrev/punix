#ifndef _TTY_H_
#define _TTY_H_

struct tty {
	struct queue t_rawq;
	struct queue t_canq;
	struct queue t_outq;
	int t_state;
	int t_delct;
	struct termios t_termios;
	dev_t t_dev;
	pid_t t_pgrp;
};

#define TTIPRI 10
#define TTOPRI 20

/* default special characters */
#define CERASE 0x7f        /* DEL */
#define CEOT   ('D'-0x40)  /* Ctrl-D */
#define CKILL  ('U'-0x40)  /* Ctrl-U */
#define CQUIT  ('\\'-0x40) /* Ctrl-\ */
#define CINTR  ('C'-0x40)  /* Ctrl-C */
#define CSTOP  ('S'-0x40)  /* Ctrl-S */
#define CSTART ('Q'-0x40)  /* Ctrl-Q */

#define ISOPEN 04
#define TTSTOP 0400

#endif
