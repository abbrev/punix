#ifndef _H_WAIT_H_
#define _H_WAIT_H_

#define W_STATUS(sig,ret) ((ret << 8) | (sig))

#define W_EXITCODE(ret) W_STATUS(0,ret)
#define W_STOPCODE(sig) W_STATUS(sig,0x7f)
#define W_TERMCODE(sig) W_STATUS(sig,0)
#define W_CONTCODE()    W_STATUS(0xff,0)

#endif

