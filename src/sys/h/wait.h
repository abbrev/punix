#ifndef _H_WAIT_H_
#define _H_WAIT_H_

#define W_STATUS(ret,type) (((ret) << 8) | ((type) & 0xff))

#define W_EXITCODE(ret) W_STATUS(ret,0)
#define W_TERMCODE(sig) W_STATUS(sig,1)
#define W_STOPCODE(sig) W_STATUS(sig,2)
#define W_CONTCODE()    W_STATUS(  0,3)

#endif

