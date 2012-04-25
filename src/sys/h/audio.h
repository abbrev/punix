#ifndef _AUDIO_H_
#define _AUDIO_H_

#define LOG2AUDIOQSIZE 7
#define AUDIOQSIZE (1<<LOG2AUDIOQSIZE)

void audioinit();
void audioopen(dev_t dev, int rw);
void audioclose(dev_t dev, int flag);
void audioread(dev_t dev);
void audiowrite(dev_t dev);
void audiointr();
void audioioctl(dev_t dev, int cmd, void *cmarg, int flag);

#endif
