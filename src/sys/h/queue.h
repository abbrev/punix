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
/*
 * q_head points to the next available slot for a byte
 * q_tail points to the next available byte
 * both q_head and q_tail are post-incremented
 */
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

int qputc(int ch, struct queue *qp);
int qunputc(struct queue *qp);
int qgetc(struct queue *qp);
int qungetc(int ch, struct queue *qp);

/* copy buffer to queue. return number of bytes copied */
int b_to_q(char *bp, int count, struct queue *qp);
/* copy queue to buffer. return number of bytes copied */
int q_to_b(struct queue *qp, char *bp, int count);
/* concatenate from queue 'from' to queue 'to' */
void catq(struct queue *from, struct queue *to);


#endif
