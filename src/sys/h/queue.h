#ifndef _QUEUE_H_
#define _QUEUE_H_

/* 512 is the minimum queue size because that is the guaranteed minimum atomic
 * write size for a pipe. */
#define QSIZE 512

#if 0
struct queue {
	int q_count;
	char *q_head, *q_tail; /* write to the head, read from the tail */
	char q_buf[QSIZE];
};
#else
struct queue {
	int q_count;
	int q_head, q_tail;
	char q_buf[QSIZE];
};
#endif

int qisfull(struct queue *qp);
int qisempty(struct queue *qp);
#define qisfull(qp) ((qp)->q_count >= QSIZE)
#define qisempty(qp) ((qp)->q_count == 0)
void qclear(struct queue *qp);

int putc(int ch, struct queue *qp);
int getc(struct queue *qp);

#endif
