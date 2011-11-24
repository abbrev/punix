#ifndef _CONTEXT_H_
#define _CONTEXT_H_

struct context {
	void **dreg[5];  /* %d3-%d7 */
	void **usp;     /* %usp */
	void **areg[3];  /* %a2-%a4 */
	void **a5;
	void **fp;
	void **sp;      /* %a7 */
	/* note: the following two align with a trap stack frame */
	short sr;      /* %sr */
	void *pc;      /* return address */
};

int csave(struct context *ctx);
void crestore(struct context *ctx);

#endif
