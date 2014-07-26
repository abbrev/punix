#ifndef _WORDEXP_H_
#define _WORDEXP_H_

#include <stddef.h>

struct wordexp_t {
	size_t	we_wordc;	/* count of words matched by words */
	char	**we_wordv;	/* pointer to list of expanded words */
	size_t	we_offs;	/* slots to reserve at the beginning of we_wordv */
};

/* 'flags' argument of wordexp() */
#define WRDE_APPEND	0x01
#define WRDE_DOOFFS	0x02
#define WRDE_NOCMD	0x04
#define WRDE_REUSE	0x08
#define WRDE_SHOWERR	0x10
#define WRDE_UNDEF	0x20

/* error return values */
#define WRDE_BADCHAR	1
#define WRDE_BADVAL	2
#define WRDE_CMDSUB	3
#define WRDE_NOSPACE	4
#define WRDE_NOSYS	5
#define WRDE_SYNTAX	6

int	wordexp(const char *__words, wordexp_t *__pwordexp, int __flags);
void	wordfree(wordexp_t *__pwordexp);

#endif
