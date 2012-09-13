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

#define UPDATE_TIMEOUT (HZ/4)

void updaterxtx(void *unused)
{
	(void)unused;
	G.link.rxtx = 0;
	timeout(updaterxtx, NULL, UPDATE_TIMEOUT);
}

STARTUP(void linkinit())
{
	qinit(&G.link.readq.q, LOG2LINKQSIZE);
	qinit(&G.link.writeq.q, LOG2LINKQSIZE);
#if 0
	kprintf("link: readq:  sizeof=%ld qsize=%d qmask=%04x\n",
	        sizeof(G.link.readq), qsize(&G.link.readq.q), qmask(&G.link.readq.q));
	kprintf("link: writeq: sizeof=%ld qsize=%d qmask=%04x\n",
	        sizeof(G.link.writeq), qsize(&G.link.writeq.q), qmask(&G.link.writeq.q));
#endif
	G.link.lowat = G.link.hiwat = -1;
	G.link.readoverflow = 0;
	ioport = 0;
	G.link.open = 0;
	LINK_CONTROL = LC_DIRECT | LC_TODISABLE;
	while (untimeout(updaterxtx, NULL))
		;
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
	while (!qisempty(&G.link.writeq.q)) {
		G.link.lowat = 0;
		txon();
		slp(&G.link.writeq.q, 0);
	}
	splx(x);
}

/* get a byte from the link read buffer and put it on the receive queue */
STARTUP(static void recvbyte())
{
	int ch;
	ch = LINK_BUFFER;
	qputc(ch, &G.link.readq.q);
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
		/* do nothing */
		return;
	}
	
	if (status & LS_ERROR) {
#if 0
	++*(short *)(0x4c00+0xf00-30*3);
#endif
		/* acknowledge the error */
		
		/* LC_AUTOSTART | LC_DIRECT | LC_TODISABLE */
		LINK_CONTROL = 0xe0;
		
		/* LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX | LC_TRIGTX */
		LINK_CONTROL = 0x8f;
		
		qclear(&G.link.writeq.q);
		txoff();
		defer(wakeup, &G.link.writeq.q);
		/* return; */
	}
	
	if ((status & LS_RXBYTE)) {
#if 0
	++*(short *)(0x4c00+0xf00-30*4);
#endif
		int x = qfree(&G.link.readq.q);
		//kprintf("Rx ");
		if (!G.link.open) {
			//kprintf("not-open ");
			rxoff();
			ch = LINK_BUFFER; /* discard it */
		} else if (x == 0) { /* no room for this byte */
			//kprintf("<.. ");
			/*
			 * N.B. The link hardware will not interrupt us again
			 * for LS_RXBYTE until after we read the existing byte.
			 * Set readoverflow so linkread gets the byte and
			 * resumes receiving.
			 */
			G.link.readoverflow = 1;
			rxoff();
		} else {
			recvbyte();
			G.link.rxtx |= 2;
			
			if (G.link.hiwat >= 0 &&
			    qused(&G.link.readq.q) >= G.link.hiwat) {
				/* wakeup any processes reading from the link */
				defer(wakeup, &G.link.readq.q);
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
		} else if ((ch = qgetc(&G.link.writeq.q)) < 0) { /* nothing to send */
			//kprintf(">.. ");
			//kprintf("empty ");
			txoff();
		} else {
			LINK_BUFFER = ch;
			G.link.rxtx |= 1;
			//kprintf(">0x%02x ", ch);
			
			if (qused(&G.link.writeq.q) <= G.link.lowat) {
				/* wakeup any procs writing to the link */
				defer(wakeup, &G.link.writeq.q);
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
	
	qclear(&G.link.readq.q);
	qclear(&G.link.writeq.q);
	G.link.lowat = G.link.hiwat = -1;
	LINK_CONTROL = G.link.control = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX;
	++ioport; /* block other uses of the IO port */
	G.link.open = 1;
	return 0;
}

/*
 * XXX: we shouldn't flush here because a process will stay in an
 * uninterruptible sleep state until the write queue is drained, which might
 * be never if nobody else is connected to the link port.
 * Perhaps a new ioctl could drain the write queue, but it would be
 * interruptible so the process can be killed.
 */
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
		while ((ch = qgetc(&G.link.readq.q)) < 0) {
#if 1
			if (G.link.readoverflow) {
				G.link.readoverflow = 0;
				recvbyte();
				continue;
			}
#endif
			rxon();
			if (oldcount != count) {
				/* we got some data already, so just return */
				goto out;
			}
			x = spl4();
			G.link.hiwat = 1;
			slp(&G.link.readq.q, 1);
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
		while (qputc(ch, &G.link.writeq.q) < 0) {
			txon();
			G.link.lowat = LINKQSIZE - 32; /* XXX constant */
			slp(&G.link.writeq.q, 1);
		}
		splx(x);
	}
	return oldcount - count;
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
