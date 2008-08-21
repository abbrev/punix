#ifndef _SYS_PROCESS_H_
#define _SYS_PROCESS_H_

#include "proc.h"

extern void slp(void *, int);

void swtch();
void psignal(struct proc *p, int sig);
void slp(void *event, int pri);
void wakeup(void *event);
struct proc *palloc();
void pfree(struct proc *);
int pidalloc();

#endif
