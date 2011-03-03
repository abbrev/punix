#ifndef _EXECINFO_H_
#define _EXECINFO_H_

int stacktrace(void **buffer, int size);
int backtrace(void **buffer, int size);

#endif
