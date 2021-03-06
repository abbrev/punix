#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "heap.h"

/*
 * q_head%size points to the next available slot for a byte
 * q_tail%size points to the next available byte
 * both q_head and q_tail are post-incremented when putting a character in the
 * normal direction
 */
typedef struct queue {
	unsigned q_head, q_tail;
	unsigned q_mask;
	char q_buf[];
} queue;

#define QUEUE(log2size) union { queue q; char dummy[sizeof(queue)+(1<<log2size)]; }

static inline int qmask(queue const *q) { return q->q_mask; }
static inline int qsize(queue const *q) { return q->q_mask + 1; }
static inline int qused(queue const *q) { return (q->q_head - q->q_tail); }
static inline int qfree(queue const *q) { return (qsize(q) - qused(q)); }
static inline int qisfull(queue const *q) { return (qfree(q) == 0); }
static inline int qisempty(queue const *q) { return (qused(q) == 0); }
static inline void qclear(queue *q) { (q)->q_tail = (q)->q_head = 0; }

#define QLOG2MAXSIZE 14 // limit size to 16KB

static inline void qinit(queue *q, unsigned log2size)
{
	unsigned size;
	if (log2size > QLOG2MAXSIZE)
		log2size = QLOG2MAXSIZE;
	size = 1 << log2size;
	q->q_mask = size - 1;
	qclear(q);
}

static inline queue *qnew(unsigned log2size)
{
	queue *q;
	size_t size, qsize;
	if (log2size > QLOG2MAXSIZE)
		log2size = QLOG2MAXSIZE;
	size = 1 << log2size;
	qsize = size + sizeof(queue);
	q = memalloc(&qsize, 0);
	if (q)
		qinit(q, log2size);
	return q;
}

static inline int qputc_no_lock(int ch, queue *qp)
{
	if (qisfull(qp))
		return -1;
	
	qp->q_buf[qp->q_head++ & qp->q_mask] = ch;

	return (unsigned char)ch;
}

#include <assert.h>

static inline int qputc(int ch, queue *qp)
{
	assert(qp->q_mask);
	int x = spl5();
	int c = qputc_no_lock(ch, qp);
	splx(x);
	return c;
}

static inline int qunputc_no_lock(queue *qp)
{
	int ch;
	if (qisempty(qp))
		return -1;

	ch = qp->q_buf[--qp->q_head & qp->q_mask];

	return (unsigned char)ch;
}

static inline int qunputc(queue *qp)
{
	int x = spl5();
	int c = qunputc_no_lock(qp);
	splx(x);
	return c;
}

static inline int qgetc_no_lock(queue *qp)
{
	int ch;
	if (qisempty(qp))
		return -1;

	ch = qp->q_buf[qp->q_tail++ & qp->q_mask];

	return (unsigned char)ch;
}

static inline int qgetc(queue *qp)
{
	int x = spl5();
	int c = qgetc_no_lock(qp);
	splx(x);
	return c;
}

static inline int qungetc_no_lock(int ch, queue *qp)
{
	if (qisfull(qp))
		return -1;

	qp->q_buf[--qp->q_tail & qp->q_mask] = ch;

	return (unsigned char)ch;
}

static inline int qungetc(int ch, queue *qp)
{
	int x = spl5();
	int c = qungetc_no_lock(ch, qp);
	splx(x);
	return c;
}

/* copy buffer to queue. return number of bytes copied */
int b_to_q(char const *bp, int count, queue *qp);
/* copy queue to buffer. return number of bytes copied */
int q_to_b(queue *qp, char *bp, int count);
/* concatenate from queue 'from' to queue 'to' */
void catq(queue *from, queue *to);


#endif
