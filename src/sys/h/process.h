#ifndef _SYS_PROCESS_H_
#define _SYS_PROCESS_H_

#include "proc.h"

void swtch();
void psignal(struct proc *p, int sig);
int tsleep(void *chan, int intr, long timo);
void slp(void *event, int pri);
void wakeup(void *event);
struct proc *palloc();
void pfree(struct proc *);
int pidalloc();

#endif
