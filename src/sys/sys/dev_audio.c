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
		if ((c = qgetc(&G.audio.q)) < 0)
			return;
		G.audio.samp = c;
		G.audio.samples = SAMPLESPERBYTE;
	}
	
	/* put this sample into the lower 2 bits */
	G.audio.samp = ROLB(G.audio.samp, 2);
	--G.audio.samples;
	
	++G.audio.optr;
}

STARTUP(int audio_open(struct file *fp, struct inode *ip))
{
	if (ioport) {
		P.p_error = EBUSY;
		return -1;
	}
	
	++ioport; /* one reference for being open */
	
	qclear(&G.audio.q);
	startaudio();

	return 0;
}

STARTUP(int audio_close(struct file *fp))
{
	dspsync();
	stopaudio();
	ioport = 0; /* no longer open */

	return 0;
}

/* there is no reading from the audio device */
STARTUP(ssize_t audio_read(struct file *fp, void *buf, size_t count, off_t *pos))
{
	return 0;
}

/*
 * MAXCOPYSIZE is the maximum number of bytes that we copy at a time from the
 * audio buffer provided by the user to the audio queue. This ensures that the
 * audio interrupt can run at least once between each time we write to the
 * audio queue (we have only about 12e6/8192 = 1464 cycles between each audio
 * interrupt to copy data to the audio queue).
 */
#define MAXCOPYSIZE 16

/* write audio samples to the audio queue */
STARTUP(ssize_t audio_write(struct file *fp, void *buf, size_t count, off_t *pos))
{
	int x;
	size_t oldcount = count;

	while (count) {
	
		size_t n;
		n = b_to_q(buf, MIN(count, MAXCOPYSIZE), &G.audio.q);
		buf += n;
		count -= n;
		if (n == 0) {
			if (!G.audio.play) /* playback is halted */
				return oldcount - count;
			
			x = spl5();
			G.audio.lowat = QSIZE - MAXCOPYSIZE;
			
			slp(&G.audio.q, 1);
			splx(x);
		}
	}
	return count;
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
STARTUP(int audio_ioctl(struct file *fp, int request, void *arg))
{
	int x;
	/*struct oss_count_t count;*/
	int speed;
	
	switch (request) {
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
		copyout(arg, &count);
		break;
	case SNDCTL_DSP_HALT:
	case SNDCTL_DSP_HALT_OUTPUT:
		stopaudio();
		break;
#endif
	case SNDCTL_DSP_SPEED:
		speed = SAMPLESPERSEC;
		copyout(arg, &speed, sizeof(speed));
		break;
	case SNDCTL_DSP_SYNC:
		dspsync();
		break;
	default:
		P.p_error = ENOSYS;
	}
	return 0;
}
#else
STARTUP(void audioioctl(dev_t dev, int cmd, void *cmarg, int flag))
{
}
#endif

off_t pipe_lseek(struct file *, off_t, int);
const struct fileops audio_fileops = {
	.open = audio_open,
	.close = audio_close,
	.read = audio_read,
	.write = audio_write,
	.lseek = pipe_lseek,
	.ioctl = audio_ioctl,
};
