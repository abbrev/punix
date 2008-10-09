#ifndef _H_WAIT_H_
#define _H_WAIT_H_

#define W_STOPCODE(sig) ((sig << 8) | 0177)
#define W_EXITCODE(ret,sig) ((ret << 8) | (sig))

#endif

