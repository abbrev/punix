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
#include "process.h"

#define UPDATE_TIMEOUT (HZ/8)

void updaterxtx(void *unused)
{
	(void)unused;
	G.link.rxtx = 0;
	timeout(updaterxtx, NULL, UPDATE_TIMEOUT);
}

STARTUP(void linkinit())
{
	qclear(&G.link.readq);
	qclear(&G.link.writeq);
	G.link.lowat = G.link.hiwat = -1;
	G.link.readoverflow = 0;
	ioport = 0;
	G.link.open = 0;
	LINK_CONTROL = LC_DIRECT | LC_TODISABLE;
	updaterxtx(NULL);
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
	
	//kprintf("[ ");
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
	
	if ((status & LS_RXBYTE)) {
#if 0
	++*(short *)(0x4c00+0xf00-30*4);
#endif
		int x = qfree(&G.link.readq);
		//kprintf("Rx ");
		if (!G.link.open) {
			//kprintf("not-open ");
			rxoff();
			ch = LINK_BUFFER; /* discard it */
		} else if (x == 0) { /* no room for this byte */
			//kprintf("<.. ");
			//kprintf("full ");
			rxoff();
		} else {
			if (x == 1) {
				//kprintf("almost-full ");
				rxoff();
			}
			recvbyte();
			G.link.rxtx |= 2;
			
			if (G.link.hiwat >= 0 &&
			    qused(&G.link.readq) >= G.link.hiwat) {
				/* wakeup any processes reading from the link */
				defer(wakeup, &G.link.readq);
				G.link.hiwat = -1;
			}
		}
	}
	
	if ((status & LS_TXEMPTY)) {
#if 0
	++*(short *)(0x4c00+0xf00-30*5);
#endif
		/* send the next byte from the send queue */
		
		//kprintf("Tx ");
		if (!G.link.open) {
			//kprintf("not-open ");
			txoff();
		} else if ((ch = qgetc(&G.link.writeq)) < 0) { /* nothing to send */
			//kprintf(">.. ");
			//kprintf("empty ");
			txoff();
		} else {
			LINK_BUFFER = ch;
			G.link.rxtx |= 1;
			//kprintf(">0x%02x ", ch);
			
			if (qused(&G.link.writeq) <= G.link.lowat) {
				/* wakeup any procs writing to the link */
				defer(wakeup, &G.link.writeq);
				G.link.lowat = -1;
			}
		}
	}
	//kprintf("] ");
}

STARTUP(int link_open(struct file *fp, struct inode *ip))
{
	if (ioport) {
		P.p_error = EBUSY;
		return -1;
	}
	
	qclear(&G.link.readq);
	qclear(&G.link.writeq);
	G.link.lowat = G.link.hiwat = -1;
	LINK_CONTROL = G.link.control = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX;
	++ioport; /* block other uses of the IO port */
	G.link.open = 1;
	return 0;
}

STARTUP(int link_close(struct file *fp))
{
	flush();
	linkinit();
	return 0;
}

// XXX link device should be a tty
// we need to use ttyread and stuff here
STARTUP(ssize_t link_read(struct file *fp, void *buf, size_t count, off_t *pos))
{
	/* read up to p_count bytes from the rx queue to p_base */
	int ch;
	int x;
	size_t oldcount = count;
	
	while (count) {
		rxon();
		while ((ch = qgetc(&G.link.readq)) < 0) {
			rxon();
#if 0
			if (G.link.readoverflow) {
				G.link.readoverflow = 0;
				recvbyte();
				continue;
			}
#endif
			if (oldcount != count) {
				/* we got some data already, so just return */
				goto out;
			}
			x = spl4();
			G.link.hiwat = 1;
			slp(&G.link.readq, 1);
			splx(x);
		}
		*(char *)buf++ = ch;
		--count;
	}
out:
	return oldcount - count;
}

STARTUP(ssize_t link_write(struct file *fp, void *buf, size_t count, off_t *pos))
{
	int ch;
	int x;
	size_t oldcount = count;
	
	for (; count; --count) {
		ch = *(char *)buf++;
		x = spl4();
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
int generic_file_fstat(struct file *fp, struct stat *buf);
const struct fileops link_fileops = {
	.open = link_open,
	.close = link_close,
	.read = link_read,
	.write = link_write,
	.lseek = pipe_lseek,
	.ioctl = link_ioctl,
	.fstat = generic_file_fstat,
};
