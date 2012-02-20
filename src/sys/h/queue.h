#ifndef _QUEUE_H_
#define _QUEUE_H_

/* 512 is the minimum queue size because that is the guaranteed minimum atomic
 * write size for a pipe. */
#define QSIZE 1024
#define QMASK (QSIZE - 1)

/*
 * q_head%QSIZE points to the next available slot for a byte
 * q_tail%QSIZE points to the next available byte
 * both q_head and q_tail are post-incremented when putting a character in the
 * normal direction
 */
struct queue {
	unsigned long q_head, q_tail;
	char q_buf[QSIZE];
};

static inline int qused(struct queue const *q) { return ((q)->q_head - (q)->q_tail); }
static inline int qfree(struct queue const *q) { return (QSIZE - qused(q)); }
static inline int qisfull(struct queue const *q) { return (qfree(q) == 0); }
static inline int qisempty(struct queue const *q) { return (qused(q) == 0); }
static inline void qclear(struct queue *q) { (q)->q_tail = (q)->q_head = 0; }

static inline int qputc_no_lock(int ch, struct queue *qp)
{
	if (qisfull(qp))
		return -1;
	
	qp->q_buf[qp->q_head++ & QMASK] = ch;

	return (unsigned char)ch;
}

static inline int qputc(int ch, struct queue *qp)
{
	int x = spl5();
	int c = qputc_no_lock(ch, qp);
	splx(x);
	return c;
}

static inline int qunputc_no_lock(struct queue *qp)
{
	int ch;
	if (qisempty(qp))
		return -1;

	ch = qp->q_buf[--qp->q_head & QMASK];

	return (unsigned char)ch;
}

static inline int qunputc(struct queue *qp)
{
	int x = spl5();
	int c = qunputc_no_lock(qp);
	splx(x);
	return c;
}

static inline int qgetc_no_lock(struct queue *qp)
{
	int ch;
	if (qisempty(qp))
		return -1;

	ch = qp->q_buf[qp->q_tail++ & QMASK];

	return (unsigned char)ch;
}

static inline int qgetc(struct queue *qp)
{
	int x = spl5();
	int c = qgetc_no_lock(qp);
	splx(x);
	return c;
}

static inline int qungetc_no_lock(int ch, struct queue *qp)
{
	if (qisfull(qp))
		return -1;

	qp->q_buf[--qp->q_tail & QMASK] = ch;

	return (unsigned char)ch;
}

static inline int qungetc(int ch, struct queue *qp)
{
	int x = spl5();
	int c = qungetc_no_lock(ch, qp);
	splx(x);
	return c;
}

/* copy buffer to queue. return number of bytes copied */
int b_to_q(char const *bp, int count, struct queue *qp);
/* copy queue to buffer. return number of bytes copied */
int q_to_b(struct queue *qp, char *bp, int count);
/* concatenate from queue 'from' to queue 'to' */
void catq(struct queue *from, struct queue *to);


#endif
