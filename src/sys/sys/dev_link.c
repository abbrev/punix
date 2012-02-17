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

STARTUP(void linkopen(dev_t dev, int rw))
{
	if (ioport) {
		P.p_error = EBUSY;
		return;
	}
	
	qclear(&G.link.readq);
	qclear(&G.link.writeq);
	G.link.lowat = G.link.hiwat = -1;
	LINK_CONTROL = G.link.control = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX;
	++ioport; /* block other uses of the IO port */
	G.link.open = 1;
}

STARTUP(void linkclose(dev_t dev, int flag))
{
	flush();
	linkinit();
}

STARTUP(void linkread(dev_t dev))
{
	/* read up to p_count bytes from the rx queue to p_base */
	int ch;
	int x;
	size_t count = P.p_count;
	
	while (P.p_count) {
		x = spl4();
		if (G.link.readoverflow) {
			G.link.readoverflow = 0;
			recvbyte();
		}
		rxon();
		while ((ch = qgetc(&G.link.readq)) < 0) {
			rxon();
			if (count == P.p_count) {
				G.link.hiwat = 1;
				slp(&G.link.readq, 1);
			} else {
				/* we got some data already, so just return */
				return;
			}
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
		x = spl4();
		txon();
		while (qputc(ch, &G.link.writeq) < 0) {
			txon();
			G.link.lowat = QSIZE - 32; /* XXX constant */
			slp(&G.link.writeq, 1);
		}
		splx(x);
	}
}

STARTUP(void linkioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
}
