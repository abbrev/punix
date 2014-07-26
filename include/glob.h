#ifndef _GLOB_H_
#define _GLOB_H_

typedef struct {
	size_t	gl_pathc;	/* count of paths matched by pattern */
	char	**gl_pathv;	/* pointer to a list of matched pathnames */
	size_t	gl_offs;	/* slots to reserve at the beginning of gl_pathv */
} glob_t;

/* values for 'flags' argument */
#define GLOB_APPEND	0x01
#define GLOB_DOOFFS	0x02
#define GLOB_ERR	0x04
#define GLOB_MARK	0x08
#define GLOB_NOCHECK	0x10
#define GLOB_NOESCAPE	0x20
#define GLOB_NOSORT	0x40
	
/* error return values */
#define GLOB_ABORTED	1
#define GLOB_NOMATCH	2
#define GLOB_NOSPACE	3

int	glob(const char *__pattern, int __flags, int (*__errfunc)(const char *__epath, int __eerrno), glob_t *__pglob);
void	globfree(glob_t *__pglob);

#endif
