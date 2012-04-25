#ifndef _LINK_H_
#define _LINK_H_

#define LOG2LINKQSIZE 9
#define LINKQSIZE (1<<LOG2LINKQSIZE)

void linkinit();
void linkopen(dev_t dev, int rw);
void linkclose(dev_t dev, int flag);
void linkread(dev_t dev);
void linkwrite(dev_t dev);
void linkintr();
void linkioctl(dev_t dev, int cmd, void *cmarg, int flag);

#endif
