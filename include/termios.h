#ifndef _TERMIOS_H_
#define _TERMIOS_H_

/* $Id$ */

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 32
struct termios {
	tcflag_t	c_iflag;	/* input modes */
	tcflag_t	c_oflag;	/* output modes */
	tcflag_t	c_cflag;	/* control modes */
	tcflag_t	c_lflag;	/* local modes */
	cc_t		c_cc[NCCS];	/* control characters */
	/* following fields are not in POSIX */
	cc_t		c_line;		/* line discipline */
	speed_t		c_ispeed;	/* input speed */
	speed_t		c_ospeed;	/* output speed */
};

/* c_cc characters (subscript for c_cc array) */
#define VINTR    0   /* */
#define VQUIT    1   /* */
#define VERASE   2   /* */
#define VKILL    3   /* */
#define VEOF     4   /* */
#define VTIME    5   /* */
#define VMIN     6   /* */
#define VSWTC    7   /* not in POSIX */
#define VSTART   8   /* */
#define VSTOP    9   /* */
#define VSUSP    10  /* */
#define VEOL     11  /* */
#define VREPRINT 12  /* not in POSIX */
#define VDISCARD 13  /* not in POSIX */
#define VWERASE  14  /* not in POSIX */
#define VLNEXT   15  /* not in POSIX */
#define VEOL2    16  /* not in POSIX */

/* c_iflag bits */
#define IGNBRK  0000001  /* Ignore break condition. */
#define BRKINT  0000002  /* Signal interrupt on break. */
#define IGNPAR  0000004  /* Ignore characters with parity errors. */
#define PARMRK  0000010  /* Mark parity errors. */
#define INPCK   0000020  /* Enable input parity check. */
#define ISTRIP  0000040  /* Strip character. */
#define INLCR   0000100  /* Map NL to CR on input. */
#define IGNCR   0000200  /* Ignore CR. */
#define ICRNL   0000400  /* Map CR to NL on input. */
#define IXON    0001000  /* Enable start/stop output control */
#define IXANY   0002000  /* Enable any character to restart output. */
#define IXOFF   0004000  /* Enable start/stop input control */

/* c_oflag bits */
#define OPOST   0000001  /* Post-process output. */
#define ONLCR   0000002  /* Map NL to CR-NL on output. (XSI) */
#define OCRNL   0000004  /* Map CR to NL on output. */
#define ONOCR   0000010  /* No CR output at column 0. */
#define ONLRET  0000020  /* NL performs CR function. */
#define OFILL   0000040  /* Use fill characters for delay. */

#define NLDLY   0000400  /* Select newline delays: */
#define   NL0   0000000  /* Newline type 0. */
#define   NL1   0000400  /* Newline type 1. */
#define CRDLY   0003000  /* Select carriage-return delays: */
#define   CR0   0000000  /* Carriage-return delay type 0. */
#define   CR1   0001000  /* Carriage-return delay type 1. */
#define   CR2   0002000  /* Carriage-return delay type 2. */
#define   CR3   0003000  /* Carriage-return delay type 3. */
#define TABDLY  0014000  /* Select horizontal-tab delays: */
#define   TAB0  0000000  /* Horizontal-tab delay type 0. */
#define   TAB1  0004000  /* Horizontal-tab delay type 1. */
#define   TAB2  0010000  /* Horizontal-tab delay type 2. */
#define   TAB3  0014000  /* Horizontal-tab delay type 3. */
#define BSDLY   0020000  /* Select backspace delays: */
#define   BS0   0000000  /* Backspace delay type 0. */
#define   BS1   0020000  /* Backspace delay type 1. */
#define FFDLY   0100000  /* Select form-feed delays: */
#define   FF0   0000000  /* Form-feed delay type 0. */
#define   FF1   0100000  /* Form-feed delay type 1. */
#define VTDLY   0040000  /* Select vertical-tab delays: */
#define   VT0   0000000  /* Vertical-tab delay type 0. */
#define   VT1   0040000  /* Vertical-tab delay type 1. */

/* c_cflag bit meaning */
#ifdef __USE_MISC
# define CBAUD  0010017
#endif
#define  B0     0000000  /* Hang up */
#define  B50    0000001  /* 50 baud */
#define  B75    0000002  /* 75 baud */
#define  B110   0000003  /* 110 baud */
#define  B134   0000004  /* 134.5 baud */
#define  B150   0000005  /* 150 baud */
#define  B200   0000006  /* 200 baud */
#define  B300   0000007  /* 300 baud */
#define  B600   0000010  /* 600 baud */
#define  B1200  0000011  /* 1200 baud */
#define  B1800  0000012  /* 1800 baud */
#define  B2400  0000013  /* 2400 baud */
#define  B4800  0000014  /* 4800 baud */
#define  B9600  0000015  /* 9600 baud */
#define  B19200 0000016  /* 19200 baud */ 
#define  B38400 0000017  /* 38400 baud */ 

/* flag bits for c_cflag */
#define CSIZE   0000060  /* Character size: */
#define   CS5   0000000  /* 5 bits */
#define   CS6   0000020  /* 6 bits */
#define   CS7   0000040  /* 7 bits */
#define   CS8   0000060  /* 8 bits */
#define CSTOPB  0000100  /* Send two stop bits, else one. */
#define CREAD   0000200  /* Enable receiver. */
#define PARENB  0000400  /* Parity enable. */
#define PARODD  0001000  /* Odd parity, else even. */
#define HUPCL   0002000  /* Hang up on last close. */
#define CLOCAL  0004000  /* Ignore modem status lines. */

/* c_lflag bits */
#define ISIG    0000001 /* Enable signals. */
#define ICANON  0000002 /* Canonical input (erase and kill processing). */
#if defined __USE_MISC || defined __USE_XOPEN
# define XCASE  0000004  /* OBSOLETE */
#endif
#define ECHO    0000010  /* Enable echo. */
#define ECHOE   0000020  /* Echo erase character as error-correcting backspace. */
#define ECHOK   0000040  /* Echo KILL. */
#define ECHONL  0000100  /* Echo NL. */
#define NOFLSH  0000200  /* Disable flush after interrupt or quit. */
#define TOSTOP  0000400  /* Send SIGTTOU for background output. */
#define IEXTEN  0100000  /* Enable extended input character processing. */

/* tcsetattr() uses these */
#define TCSANOW   0  /* Change attributes immediately.*/
#define TCSADRAIN 1  /* Change attributes when output has drained.*/
#define TCSAFLUSH 2  /* Change attributes when output has drained;
                        also flush pending input. */

/* tcflush() uses these */
#define TCIFLUSH  0  /* Flush pending input. */
#define TCOFLUSH  1  /* Flush both pending input and untransmitted output. */
#define TCIOFLUSH 2  /* Flush untransmitted output. */

/* tcflow() uses these */
#define TCOOFF 0  /* Transmit a STOP character, intended to suspend input data. */
#define TCOON  1  /* Transmit a START character, intended to restart input data. */
#define TCIOFF 2  /* Suspend output. */
#define TCION  3  /* Restart output. */

speed_t	cfgetispeed(const struct termios *__termios_p);
speed_t	cfgetospeed(const struct termios *__termios_p);
int	cfsetispeed(struct termios *__termios_p, speed_t __speed);
int	cfsetospeed(struct termios *__termios_p, speed_t __speed);
int	tcdrain(int __fd);
int	tcflow(int __fd, int __action);
int	tcflush(int __fd, int __queue_selector);
int	tcgetattr(int __fd, struct termios *__termios_p);
pid_t	tcgetsid(int); /* [XSI] */
int	tcsendbreak(int __fd, int __duration);
int	tcsetattr(int __fd, int __optional_actions,
		const struct termios *__termios_p);

#if 0
/* not in POSIX */
extern void cfmakeraw(struct termios *__termios_p);
#endif

#endif /* _TERMIOS_H_ */
