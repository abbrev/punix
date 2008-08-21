#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <stddef.h>

int	bcmp(const void *, const void *, size_t);	/* LEGACY */
void	bcopy(const void *, void *, size_t);		/* LEGACY */
void	bzero(void *, size_t);				/* LEGACY */
int	ffs(int);
char	*index(const char *, int);			/* LEGACY */
char	*rindex(const char *, int);			/* LEGACY */
int	strcasecmp(const char *__s1, const char *__s2);
int	strncasecmp(const char *__s1, const char *__s2, size_t __n);

#define index		strchr
#define rindex		strrchr
#define stricmp		strcasecmp
#define strnicmp	strncasecmp

#endif
