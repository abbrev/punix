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
		slp(&G.link.writeq);
	}
	splx(x);
}

/* get a byte from the link read buffer and put it on the receive queue */
STARTUP(static void recvbyte())
{
	int ch;
	ch = LINK_BUFFER;
	putc(ch, &G.link.readq);
	//kprintf("<0x%02x ", ch);
}

STARTUP(void linkintr())
{
	int status;
	int ch;
	
	++*(short *)(0x4c00+0xf00-30*1);
	(void)LINK_CONTROL; /* read to acknowledge interrupt */
	
	status = LINK_STATUS;
	
	if (status & LS_ACTIVITY) {
	++*(short *)(0x4c00+0xf00-30*2);
		//kprintf("ac ");
		/* do nothing */
		return;
	}
	
	if (status & LS_ERROR) {
	++*(short *)(0x4c00+0xf00-30*3);
		//kprintf("er ");
		/* acknowledge the error */
		
		/* LC_AUTOSTART | LC_DIRECT | LC_TODISABLE */
		LINK_CONTROL = 0xe0;
		
		/* LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX | LC_TRIGTX */
		LINK_CONTROL = 0x8f;
		
		/* return; */
	}
	
	if ((G.link.control & LC_TRIGRX) && (status & LS_RXBYTE)) {
	++*(short *)(0x4c00+0xf00-30*4);
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
	++*(short *)(0x4c00+0xf00-30*5);
		/* send the next byte from the send queue */
		
		if ((ch = getc(&G.link.writeq)) < 0) { /* nothing to send */
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

STARTUP(void linkopen(dev_t dev, int rw))
{
	if (ioport) {
		P.p_error = EBUSY;
		return;
	}
	
	++ioport; /* block other uses of the IO port */
	
	qclear(&G.link.readq);
	qclear(&G.link.writeq);
	
	LINK_CONTROL = G.link.control = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX;
	/* XXX: anything more? */
}

STARTUP(void linkclose(dev_t dev, int flag))
{
	flush();
	qclear(&G.link.readq); /* discard any unread data */
	ioport = 0; /* free the IO port for other uses */
}

STARTUP(void linkread(dev_t dev))
{
	/* read up to p_count bytes from the rx queue to p_base */
	int ch;
	int x;
	size_t count = P.p_count;
	
	while (P.p_count) {
		x = spl5();
		while ((ch = getc(&G.link.readq)) < 0) {
			rxon();
			if (G.link.readoverflow) {
				G.link.readoverflow = 0;
				recvbyte();
				continue;
			}
			if (count == P.p_count) {
				G.link.hiwat = 1;
				slp(&G.link.readq, PPIPE);
			} else /* we got some data already, so just return */
				return;
		}
		splx(x);
		*P.p_base++ = ch;
		--P.p_count;
	}
}

STARTUP(void linkwrite(dev_t dev))
{
	int ch;
	int x;
	
	for (; P.p_count; --P.p_count) {
		ch = *P.p_base++;
		x = spl5();
		txon();
		while (putc(ch, &G.link.writeq) < 0) {
			txon();
			G.link.lowat = QSIZE - 32; /* XXX constant */
			slp(&G.link.writeq, PPIPE);
		}
		splx(x);
	}
}

STARTUP(void linkioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
}
