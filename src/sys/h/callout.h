#ifndef _CALLOUT_H_
#define _CALLOUT_H_

struct callout {
	long c_dtime;
	void *c_arg;
	void (*c_func)(void *);
};

int timeout(void (*func)(void *), void *arg, long time);
int untimeout(void (*func)(void *), void *arg);

#endif
