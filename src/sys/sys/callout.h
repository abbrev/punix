#ifndef _CALLOUT_H_
#define _CALLOUT_H_

struct callout {
	long    c_time;
	void * c_arg;
	int  (*c_func)(void *);
};

int timeout(int (*func)(void *), void *arg, long time);
void untimeout(int (*func)(void *), void *arg);

#endif
