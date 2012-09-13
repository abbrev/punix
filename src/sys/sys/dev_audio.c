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
	qinit(&G.audio.q.q, LOG2AUDIOQSIZE);
	qclear(&G.audio.q.q);
	//kprintf("&G.audio.q.q=%p\n", &G.audio.q.q);
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
	int x = spl1();  // inhibit soft interrupts
	while (G.audio.play && !qisempty(&G.audio.q.q)) {
		if (qused(&G.audio.q.q) < 4) { // XXX constant
			// samples will be played before slp() would even finish
			cpuidle(INT_ALL);
		} else {
			G.audio.lowat = 0;
			slp(&G.audio.q.q, 0);
		}
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
		if ((c = qgetc_no_lock(&G.audio.q.q)) < 0)
			goto out;
		G.audio.samp = c;
		G.audio.samples = SAMPLESPERBYTE;
		unsigned long *spinner = (unsigned long *)(0x4c00+0xf00-7*30+4*4);
		*spinner = (~0L) << 32L*qfree(&G.audio.q.q)/AUDIOQSIZE;
	}
	
	/* put this sample into the lower 2 bits */
	G.audio.samp = ROLB(G.audio.samp, 2);
	--G.audio.samples;
	
	++G.audio.optr;
out:
	if (qused(&G.audio.q.q) <= G.audio.lowat) {
		if (G.audio.lowat != 0 && qisempty(&G.audio.q.q))
			kprintf("audio buffer underrun!\n");
		G.audio.lowat = -1;
		spl4();
		defer(wakeup, &G.audio.q.q);
	}
}

STARTUP(int audio_open(struct file *fp, struct inode *ip))
{
	if (ioport) {
		P.p_error = EBUSY;
		return -1;
	}
	
	++ioport; /* one reference for being open */
	
	qclear(&G.audio.q.q);
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
#define MAXCOPYSIZE AUDIOQSIZE //8
#define MAXDRAIN (AUDIOQSIZE / 2)

/* write audio samples to the audio queue */
STARTUP(ssize_t audio_write(struct file *fp, void *buf, size_t count, off_t *pos))
{
	int x;
	size_t oldcount = count;
	size_t n;
	unsigned long *spinner = (unsigned long *)(0x4c00+0xf00-6*30+4*4);

	while (count) {
		*spinner = 0xffffffff;
		n = b_to_q(buf, MIN(count, AUDIOQSIZE), &G.audio.q.q);
		buf += n;
		count -= n;

		if (count == 0 || !G.audio.play) /* playback is halted */
			break;
			
		/*
		 * we filled the buffer practically full, so wait until it
		 * drains low enough for us to fill it up again
		 */
		*spinner = 0xffff0000;
	
		x = spl1();
		G.audio.lowat = AUDIOQSIZE - MIN(count, MAXDRAIN);
		
		slp(&G.audio.q.q, 1);
		splx(x);
	}
	return oldcount - count;
}

/* some OSS ioctl() codes to support:
 * SNDCTL_DSP_SILENCE: G.audio.q.q_count = 0;
 * SNDCTL_DSP_SKIP: G.audio.q.q_count = 0;
 * SNDCTL_DSP_SPEED: always return 8192;
 * SNDCTL_DSP_SYNC: G.audio.lowat = 0; slp(&G.audio.q.q);
 * SNDCTL_DSP_CURRENT_OPTR: return G.audio.optr and
 *   G.audio.q.q_count * SAMPLESPERBYTE;
 * SNDCTL_DSP_HALT{,_OUTPUT}
 */

#if 1
STARTUP(int audio_ioctl(struct file *fp, int request, void *arg))
{
	/*struct oss_count_t count;*/
	int speed;
	
	switch (request) {
#if 0
	case SNDCTL_DSP_SILENCE:
		qclear(&G.audio.q.q);
		qputc(0, &G.audio.q.q); /* make sure the output is 0 */
		break;
	case SNDCTL_DSP_SKIP:
		qclear(&G.audio.q.q);
		break;
	case SNDCTL_DSP_CURRENT_OPTR:
		x = spl5();
		count.samples = G.audio.optr;
		count.fifo_samples = G.audio.q.q.q_count * SAMPLESPERBYTE;
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
int generic_file_fstat(struct file *fp, struct stat *buf);
const struct fileops audio_fileops = {
	.open = audio_open,
	.close = audio_close,
	.read = audio_read,
	.write = audio_write,
	.lseek = pipe_lseek,
	.ioctl = audio_ioctl,
	.fstat = generic_file_fstat,
};
