#ifndef _SEARCH_H_
#define _SEARCH_H_

/* $Id: search.h,v 1.2 2008/01/11 13:35:45 fredfoobar Exp $ */

/* should be POSIX compliant */

#include <sys/types.h>

#ifndef __COMPAR_FN_T
#define __COMPAR_FN_T
typedef int (*__compar_fn_t)(const void *, const void *);
#endif

/* for use with hsearch(3) */

typedef struct entry {
	char *key;
	char *data;
} ENTRY;
typedef enum { FIND, ENTER } ACTION;

typedef enum { preorder, postorder, endorder, leaf } VISIT; /* For tsearch */

extern int	hcreate(size_t __nel);
extern void	hdestroy(void);
extern ENTRY	*hsearch(ENTRY __item, ACTION __action);

extern void	insque(void *__element, void *__pred);

extern void	*lfind(const void *__key, const void *__base, size_t *__nmemb,
		size_t __size, __compar_fn_t __compar);

extern void	*lsearch(const void *__key, const void *__base, size_t *__nmemb,
		size_t __size, __compar_fn_t __compar);

extern void	remque(void *__element);

/* The tsearch routines are very interesting. They make many
 * assumptions about the compiler. It assumes that the first field
 * in node must be the "key" field, which points to the datum.
 * Everything depends on that. It is a very tricky stuff. H.J.
 */

extern void *tdelete(const void *__key, void **__rootp, __compar_fn_t __compar);

extern void *tfind(const void *__key, void *const *__rootp, __compar_fn_t __compar);

extern void *tsearch(const void *__key, void **__rootp, __compar_fn_t __compar);

#ifndef __ACTION_FN_T
#define __ACTION_FN_T
/* FYI, the first argument should be a pointer to "key".
 * Please read the man page for details.
 */
typedef void (*__action_fn_t)(const void *__nodep, VISIT __value, int __level);
#endif

extern void twalk(const void *__root, __action_fn_t __action);

#endif /* _SEARCH_H_ */
