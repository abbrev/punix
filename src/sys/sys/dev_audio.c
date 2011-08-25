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
#include "process.h"

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
	LINK_CONTROL = LC_DIRECT | LC_TODISABLE;
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
	
	INT5RATE |= 0x30;
	INT5VAL = 0x00;
}

STARTUP(static void dspsync())
{
	mask(&G.calloutlock);
	while (G.audio.play && (!qisempty(&G.audio.q) || G.audio.samples)) {
		if (qused(&G.audio.q) < 4) { // XXX constant
			// samples will be played before slp() would even finish
			unmask(&G.calloutlock);
			cpuidle();
			mask(&G.calloutlock);
		} else {
			int x = spl1(); // inhibit soft interrupts
			unmask(&G.calloutlock);
			G.audio.lowat = 0;
			slp(&G.audio.q, 0);
			mask(&G.calloutlock);
			splx(x);
		}
	}
out:
	unmask(&G.calloutlock);
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
		if ((c = qgetc(&G.audio.q)) < 0)
			goto out;
		G.audio.samp = c;
		G.audio.samples = SAMPLESPERBYTE;
		unsigned long *spinner = (unsigned long *)(0x4c00+0xf00-7*30+4*4);
		*spinner = (~0L) << 32L*qfree(&G.audio.q)/QSIZE;
	}
	
	/* put this sample into the lower 2 bits */
	G.audio.samp = ROLB(G.audio.samp, 2);
	--G.audio.samples;
	
	++G.audio.optr;
out:
	if (G.audio.q.q_count <= G.audio.lowat) {
		G.audio.lowat = -1;
		spl4();
		defer(wakeup, &G.audio.q);
	}
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

/*
 * MAXCOPYSIZE is the maximum number of bytes that we copy at a time from the
 * audio buffer provided by the user to the audio queue. This ensures that the
 * audio interrupt can run at least once between each time we write to the
 * audio queue (we have only about 12e6/8192 = 1464 cycles between each audio
 * interrupt to copy data to the audio queue).
 */
#define MAXCOPYSIZE QSIZE //8
#define MAXDRAIN (QSIZE / 2)

/* write audio samples to the audio queue */
STARTUP(void audiowrite(dev_t dev))
{
	int x;
	size_t n;
	unsigned long *spinner = (unsigned long *)(0x4c00+0xf00-6*30+4*4);

	while (P.p_count) {
		*spinner = 0xffffffff;
		n = b_to_q(P.p_base, MIN(P.p_count, QSIZE), &G.audio.q);
		P.p_base += n;
		P.p_count -= n;

		if (P.p_count == 0 || !G.audio.play) /* playback is halted */
			break;

		/*
		 * we filled the buffer practically full, so wait until it
		 * drains low enough for us to fill it up again
		 */
		*spinner = 0xffff0000;
		
		x = spl1();
		G.audio.lowat = QSIZE - MIN(P.p_count, MAXDRAIN);
		
		slp(&G.audio.q, 1);
		splx(x);
	}
	*spinner = 0x00000000;
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
		qputc(0, &G.audio.q); /* make sure the output is 0 */
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
