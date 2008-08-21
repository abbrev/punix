#ifndef _QUEUE_H_
#define _QUEUE_H_

/* 512 is the minimum queue size because that is the guaranteed minimum atomic
 * write size for a pipe. */
#define QSIZE 512

struct queue {
	int q_count;
	char *q_head, *q_tail; /* write to the head, read from the tail */
	char q_buf[QSIZE];
};

int qisfull(struct queue *qp);
int qisempty(struct queue *qp);
void qclear(struct queue *qp);

int putc(int ch, struct queue *qp);
int getc(struct queue *qp);

#endif
