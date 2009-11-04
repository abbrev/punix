#ifndef _TTY_H_
#define _TTY_H_

struct tty {
	struct queue t_rawq;
	struct queue t_canq;
	struct queue t_outq;
	int t_state;
	int t_numc;
	struct termios t_termios;
	dev_t t_dev;
	pid_t t_pgrp;
};

#define TTBREAKC(c)                                                     \
	((c) == '\n' || ((c) == cc[VEOF] ||                             \
	(c) == cc[VEOL] || (c) == cc[VEOL2]) /* && (c) != _POSIX_VDISABLE*/ )

#define t_cc            t_termios.c_cc
#define t_cflag         t_termios.c_cflag
#define t_iflag         t_termios.c_iflag
#define t_ispeed        t_termios.c_ispeed
#define t_lflag         t_termios.c_lflag
#define t_min           t_termios.c_min
#define t_oflag         t_termios.c_oflag
#define t_ospeed        t_termios.c_ospeed
#define t_time          t_termios.c_time


#define TTIPRI 10
#define TTOPRI 20

#define CTRL(a) ((a)&0x1f)

/* default special characters */
#define CERASE  0x7f        /* DEL */
#define CWERASE CTRL('W')
#define CEOF    CTRL('D')
#define CEOT    CEOF
#define CEOL    '\n'
#define CKILL   CTRL('U')
#define CQUIT   CTRL('X')
#define CINTR   CTRL('C')
#define CSTOP   CTRL('S')
#define CSTART  CTRL('Q')

#define ISOPEN 04
#define TTSTOP 0400

#endif
