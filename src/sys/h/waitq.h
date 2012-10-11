#ifndef _WAITQ_H_
#define _WAITQ_H_

#include "list.h"

/* a head for waitq's (for type safety) */
struct waitq_head {
	struct list_head wq_head;
};

struct waitq {
	struct list_head wq_list;
	//int wq_flags; // this might be useful some day
	struct proc *wq_proc;
};

/* add a waitq to a wait list */
static inline void waitq_add(struct waitq *w, struct waitq_head *head)
{
	list_add_tail(&w->wq_list, &head->wq_head);
}

/* remove a waitq from its list */
static inline void waitq_del(struct waitq *w)
{
	list_del(&w->wq_list);
}

/* wake up all processes on a queue */
void waitq_wakeup_all(struct waitq_head *head);

/* wake up the first process on a queue */
void waitq_wakeup_first(struct waitq_head *head);

#define INIT_WAITQ(w) INIT_LIST_HEAD(&(w)->wq_list)
#define INIT_WAITQ_HEAD(w) INIT_LIST_HEAD(&(w)->wq_head)

#endif
