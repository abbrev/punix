#include <errno.h>
#include <sound.h>

#include "punix.h"
#include "param.h"
#include "buf.h"
#include "dev.h"
#include "ioport.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"

#define SAMPLESPERSEC 8192
#define SAMPLESPERBYTE 4
#define INT5RATE (*(volatile unsigned char *)0x600015)
#define INT5VAL  (*(volatile unsigned char *)0x600017)

/* ioport is the "reference count" of the IO port. When it is zero, the port
 * is available for any purpose. */

/* one-shot routine on system-startup */
STARTUP(void audioinit())
{
	qclear(&G.audio.q);
	ioport = 0;
}

STARTUP(static void startaudio())
{
	LINK_CONTROL = 0;
	LINK_DIRECT = 0xfc;
	INT5RATE &= ~0x30; /* set rate to OSC2 / 2^5 */
	INT5VAL = 257 - 2; /* 16384 / 2 = 8192 */
	
	G.audio.lowat = -1;
	G.audio.optr = 0;
	G.audio.play = 1;
	G.audio.samples = 0;
}

STARTUP(static void stopaudio())
{
	G.audio.play = 0;
	
	LINK_CONTROL = LC_AUTOSTART | LC_TRIGERR | LC_TRIGANY | LC_TRIGRX; /* XXX should we just let dev_link set these flags? */
	INT5RATE |= 0x30;
	INT5VAL = 0x00;
}

STARTUP(static void dspsync())
{
	int x = spl5();
	while (!qisempty(&G.audio.q) || G.audio.samples) {
		G.audio.lowat = 0; /* wait until the audio queue is empty */
		slp(&G.audio.q, 0);
	}
	splx(x);
}

/* produce the sound! */
STARTUP(void audiointr())
{
	/* this really ought to go in the assembly glue code for performance */
	if (!G.audio.play)
		return;
	
	/* emit the sample */
	LINK_DIRECT = G.audio.samp & 03;
	
	if (!G.audio.samples) {
		int c;
		if (G.audio.q.q_count <= G.audio.lowat) {
			G.audio.lowat = -1;
			wakeup(&G.audio.q);
		}
		if ((c = getc(&G.audio.q)) < 0)
			return;
		G.audio.samp = c;
		G.audio.samples = SAMPLESPERBYTE;
	}
	
	/* put this sample into the lower 2 bits */
	G.audio.samp = ROLB(G.audio.samp, 2);
	--G.audio.samples;
	
	++G.audio.optr;
}

STARTUP(void audioopen(dev_t dev, int rw))
{
	if (ioport) {
		P.p_error = EBUSY;
		return;
	}
	
	++ioport; /* one reference for being open */
	
	qclear(&G.audio.q);
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
		while (putc(ch, &G.audio.q) < 0) {
			if (!G.audio.play) /* playback is halted */
				return;
			
			x = spl5(); /* prevent the audio int from trying to wake
			             * us up before we go to sleep */
			
			/* FIXME: Set the low water value according to the
			 * amount of data we're trying to write (within upper
			 * and lower limits, of course), . */
			
			G.audio.lowat = QSIZE - 32; /* XXX constant */
			
			slp(&G.audio.q, 1);
			splx(x);
		}
	}
}

/* some OSS ioctl() codes to support:
 * SNDCTL_DSP_SILENCE: G.audio.q.q_count = 0;
 * SNDCTL_DSP_SKIP: G.audio.q.q_count = 0;
 * SNDCTL_DSP_SPEED: always return 8192;
 * SNDCTL_DSP_SYNC: G.audio.lowat = 0; slp(&G.audio.q);
 * SNDCTL_DSP_CURRENT_OPTR: return G.audio.optr and
 *   G.audio.q.q_count * SAMPLESPERBYTE;
 * SNDCTL_DSP_HALT{,_OUTPUT}
 */

#if 1
STARTUP(void audioioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
	int x;
	/*struct oss_count_t count;*/
	int speed;
	
	switch (cmd) {
#if 0
	case SNDCTL_DSP_SILENCE:
		qclear(&G.audio.q);
		putc(0, &G.audio.q); /* make sure the output is 0 */
		break;
	case SNDCTL_DSP_SKIP:
		qclear(&G.audio.q);
		break;
	case SNDCTL_DSP_CURRENT_OPTR:
		x = spl5();
		count.samples = G.audio.optr;
		count.fifo_samples = G.audio.q.q_count * SAMPLESPERBYTE;
		splx(x);
		copyout(cmarg, &count);
		break;
	case SNDCTL_DSP_HALT:
	case SNDCTL_DSP_HALT_OUTPUT:
		stopaudio();
		break;
#endif
	case SNDCTL_DSP_SPEED:
		speed = SAMPLESPERSEC;
		copyout(cmarg, &speed, sizeof(speed));
		break;
	case SNDCTL_DSP_SYNC:
		dspsync();
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
