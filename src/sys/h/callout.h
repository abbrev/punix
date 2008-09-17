#ifndef _CALLOUT_H_
#define _CALLOUT_H_

struct callout {
	long c_time;
	void *c_arg;
	void (*c_func)(void *);
};

int timeout(void (*func)(void *), void *arg, long time);
void untimeout(void (*func)(void *), void *arg);

#endif
