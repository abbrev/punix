#include <errno.h>

#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "ioport.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

/* one-shot routine on system startup */
STARTUP(void linkinit())
{
	qclear(&G.link.readq);
	qclear(&G.link.writeq);
	G.link.lowat = G.link.hiwat = -1;
	G.link.readoverflow = 0;
	ioport = 0;
}

STARTUP(static void rxon())
{
	G.link.control |= LC_TRIGRX;
	LINK_CONTROL = G.link.control;
}

STARTUP(static void rxoff())
{
	G.link.control &= ~LC_TRIGRX;
	LINK_CONTROL = G.link.control;
}

STARTUP(static void txon())
{
	G.link.control |= LC_TRIGTX;
	LINK_CONTROL = G.link.control;
}

STARTUP(static void txoff())
{
	G.link.control &= ~LC_TRIGTX;
	LINK_CONTROL = G.link.control;
}

STARTUP(static void flush())
{
	int x = spl4();
	/* wait until the write queue is empty */
	while (!qisempty(&G.link.writeq)) {
		G.link.lowat = 0;
		txon();
		slp(&G.link.writeq, 0);
	}
	splx(x);
}

/* get a byte from the link read buffer and put it on the receive queue */
STARTUP(static void recvbyte())
{
	int ch;
	ch = LINK_BUFFER;
	qputc(ch, &G.link.readq);
	//kprintf("<0x%02x ", ch);
}

STARTUP(void linkintr())
{
	int status;
	int ch;
	
#if 0
	++*(short *)(0x4c00+0xf00-30*1);
#endif
	(void)LINK_CONTROL; /* read to acknowledge interrupt */
	
	status = LINK_STATUS;
	
	if (status & LS_ACTIVITY) {
#if 0
	++*(short *)(0x4c00+0xf00-30*2);
#endif
		//kprintf("ac ");
		/* do nothing */
		return;
	}
	
	if (status & LS_ERROR) {
#if 0
	++*(short *)(0x4c00+0xf00-30*3);
#endif
		//kprintf("er ");
		/* acknowledge the error */
		
		/* LC_AUTOSTART | LC_DIRECT | LC_TODISABLE */
		LINK_CONTROL = 0xe0;
		
		/* LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX | LC_TRIGTX */
		LINK_CONTROL = 0x8f;
		
		/* return; */
	}
	
	if ((G.link.control & LC_TRIGRX) && (status & LS_RXBYTE)) {
#if 0
	++*(short *)(0x4c00+0xf00-30*4);
#endif
		if (qisfull(&G.link.readq)) { /* no room for this byte */
			//kprintf("<.. ");
			G.link.readoverflow = 1;
			rxoff();
		} else {
			recvbyte();
			
			if (G.link.hiwat >= 0 && G.link.readq.q_count >= G.link.hiwat) {
				/* wakeup any processes reading from the link */
				wakeup(&G.link.readq);
				G.link.hiwat = -1;
			}
		}
	}
	
	if ((G.link.control & LC_TRIGTX) && (status & LS_TXEMPTY)) {
#if 0
	++*(short *)(0x4c00+0xf00-30*5);
#endif
		/* send the next byte from the send queue */
		
		if ((ch = qgetc(&G.link.writeq)) < 0) { /* nothing to send */
			//kprintf(">.. ");
			txoff();
		} else {
			LINK_BUFFER = ch;
			//kprintf(">0x%02x ", ch);
			
			if (G.link.writeq.q_count <= G.link.lowat) {
				/* wakeup any procs writing to the link */
				wakeup(&G.link.writeq);
				G.link.lowat = -1;
			}
		}
	}
}

STARTUP(int link_open(struct file *fp, struct inode *ip))
{
	if (ioport) {
		P.p_error = EBUSY;
		return -1;
	}
	
	++ioport; /* block other uses of the IO port */
	
	qclear(&G.link.readq);
	qclear(&G.link.writeq);
	
	LINK_CONTROL = G.link.control = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX;

	/* XXX: anything more? */
	return 0;
}

STARTUP(int link_close(struct file *fp))
{
	flush();
	qclear(&G.link.readq); /* discard any unread data */
	ioport = 0; /* free the IO port for other uses */
	return 0;
}

// XXX link device should be a tty
// we need to use ttyread and stuff here
STARTUP(ssize_t link_read(struct file *fp, void *buf, size_t count))
{
	/* read up to p_count bytes from the rx queue to p_base */
	int ch;
	int x;
	size_t oldcount = count;
	
	while (count) {
		x = spl5();
		while ((ch = qgetc(&G.link.readq)) < 0) {
			rxon();
			if (G.link.readoverflow) {
				G.link.readoverflow = 0;
				recvbyte();
				continue;
			}
			if (oldcount == count) {
				G.link.hiwat = 1;
				slp(&G.link.readq, 1);
			} else /* we got some data already, so just return */
				return;
		}
		splx(x);
		*(char *)buf++ = ch;
		--count;
	}
	return oldcount;
}

STARTUP(ssize_t link_write(struct file *fp, void *buf, size_t count))
{
	int ch;
	int x;
	size_t oldcount = count;
	
	for (; count; --count) {
		ch = *(char *)buf++;
		x = spl5();
		txon();
		while (qputc(ch, &G.link.writeq) < 0) {
			txon();
			G.link.lowat = QSIZE - 32; /* XXX constant */
			slp(&G.link.writeq, 1);
		}
		splx(x);
	}
	return oldcount;
}

STARTUP(int link_ioctl(struct file *fp, int request, void *arg))
{
	// TODO
	P.p_error = EINVAL;
	return -1;
}

off_t pipe_lseek(struct file *, off_t, int);
const struct fileops link_fileops = {
	.open = link_open,
	.close = link_close,
	.read = link_read,
	.write = link_write,
	.lseek = pipe_lseek,
	.ioctl = link_ioctl,
};
