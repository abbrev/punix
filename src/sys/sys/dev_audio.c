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

#define SAMPLESPERBYTE 4
#define INT5RATE (*(volatile unsigned char *)0x600015)
#define INT5VAL  (*(volatile unsigned char *)0x600017)

/* ioport is the "reference count" of the IO port. When it is zero, the port
 * is available for any purpose. */

/* one-shot routine on system-startup */
STARTUP(void audioinit())
{
	qclear(&G.audioq);
}

STARTUP(static void startaudio())
{
	LINK_CONTROL = 0;
	LINK_DIRECT = 0xfc;
	INT5RATE &= ~0x30; /* set rate to OSC2 / 2^5 */
	INT5VAL = 257 - 2; /* 16384 / 2 = 8192 */
	
	G.audiolowat = -1;
	G.audiooptr = 0;
	G.audioplay = 1;
	G.audiosamples = 0;
}

STARTUP(static void stopaudio())
{
	G.audioplay = 0;
	
	LINK_CONTROL = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX; /* XXX should we just let dev_link set these flags? */
	INT5RATE |= 0x30;
	INT5VAL = 0x00;
}

STARTUP(static void dspsync())
{
	int x = spl5();
	G.audiolowat = 0; /* wait until the audio queue is empty */
	slp(&G.audioq);
	splx(x);
}

/* produce the sound! */
STARTUP(void audiointr())
{
	/* Technically, G.audiosamp and G.audiosamples should be static local
	 * variables. */
	
	/* this really ought to go in the assembly glue code for performance */
	if (!G.audioplay)
		return;
	
	if (!G.audiosamples) {
		int c;
		if ((c = getc(&G.audioq)) < 0)
			return;
		G.audiosamp = c;
		G.audiosamples = SAMPLESPERBYTE;
	}
	
	/* put this sample into the lower 2 bits */
	G.audiosamp = ROLB(G.audiosamp, 2);
	--G.audiosamples;
	
	/* emit the sample */
	LINK_DIRECT = G.audiosamp & 03;
	
	++G.audiooptr;
	
	if (G.audioq.q_count <= G.audiolowat) {
		G.audiolowat = -1;
		spl4(); /* let other audio ints occur while we do wakeup */
		wakeup(&G.audioq);
	}
}

STARTUP(void audioopen(dev_t dev, int rw))
{
	if (ioport) {
		P.p_error = EBUSY;
		return;
	}
	
	++ioport; /* one reference for being open */
	
	qclear(&G.audioq);
	startaudio();
}

STARTUP(void audioclose(dev_t dev, int flag))
{
	dspsync();
	stopaudio();
	ioport = 0; /* no longer open */
}

/* there is no reading from the audio device */
STARTUP(void audioread(dev_t dev))
{
}

/* write audio samples to the audio queue */
STARTUP(void audiowrite(dev_t dev))
{
	int ch;
	int x;
	
	for (; P.p_count; ++P.p_base, --P.p_count) {
		ch = *P.p_base;
		while (putc(ch, &G.audioq) < 0) {
			if (!G.audioplay) /* playback is halted */
				return;
			
			x = spl5(); /* prevent the audio int from trying to wake
			             * us up before we go to sleep */
			
			/* FIXME: Set the low water value according to the
			 * amount of data we're trying to write (within upper
			 * and lower limits, of course), . */
			
			G.audiolowat = QSIZE - 32; /* XXX constant */
			
			slp(&G.audioq, PAUDIO);
			splx(x);
		}
	}
}

/* some OSS ioctl() codes to support:
 * SNDCTL_DSP_SILENCE: G.audioq.q_count = 0;
 * SNDCTL_DSP_SKIP: G.audioq.q_count = 0;
 * SNDCTL_DSP_SPEED: always return 8192;
 * SNDCTL_DSP_SYNC: G.audiolowat = 0; slp(&G.audioq);
 * SNDCTL_DSP_CURRENT_OPTR: return G.audiooptr and
 *   G.audioq.q_count * SAMPLESPERBYTE;
 * SNDCTL_DSP_HALT{,_OUTPUT}
 */

#if 0
STARTUP(void audioioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
	int x;
	struct oss_count_t count;
	int speed;
	
	switch (cmd) {
	case SNDCTL_DSP_SILENCE:
		qclear(&G.audioq);
		putc(0, &G.audioq); /* make sure the output is 0 */
		break;
	case SNDCTL_DSP_SKIP:
		qclear(&G.audioq);
		break;
	case SNDCTL_DSP_SPEED:
		speed = 8192; /* XXX constant */
		copyout(cmarg, &speed);
		break;
	case SNDCTL_DSP_SYNC:
		dspsync();
		break;
	case SNDCTL_DSP_CURRENT_OPTR:
		x = spl5();
		count.samples = G.audiooptr;
		count.fifo_samples = G.audioq.q_count * SAMPLESPERBYTE;
		splx(x);
		copyout(cmarg, &count);
		break;
	case SNDCTL_DSP_HALT:
	case SNDCTL_DSP_HALT_OUTPUT:
		stopaudio();
		break;
	default:
		P.p_error = ENOSYS;
	}
}
#else
STARTUP(void audioioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
}
#endif
