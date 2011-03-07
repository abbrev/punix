#ifndef _CONTEXT_H_
#define _CONTEXT_H_

struct context {
	long dreg[5];  /* %d3-%d7 */
	long usp;      /* %usp */
	long areg[5];  /* %a2-%a6 */
	long sp;       /* %a7 */
	long pc;       /* return address */
};

int csave(struct context *ctx);
void crestore(struct context *ctx);

#endif
