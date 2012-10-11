#include "waitq.h"
#include "punix.h"
#include "proc.h"
#include "sched.h"

/* wake up all processes on a queue */
void waitq_wakeup_all(struct waitq_head *head)
{
	struct waitq *w;

	list_for_each_entry(w, &head->wq_head, wq_list) {
		sched_run(w->wq_proc);
	}
}

/* wake up the first process on a queue */
void waitq_wakeup_first(struct waitq_head *head)
{
	if (!list_empty(&head->wq_head))
		sched_run(((struct waitq *)head->wq_head.next)->wq_proc);
}

