#ifndef _STRING_H_
#define _STRING_H_

/* $Id$ */

/* mostly POSIX compliant with some extra BSD functions */

#include <stddef.h>

void	*memccpy(void *__dest, const void *__src, int __c, size_t __n);
void	*memchr(const void *__s, int __c, size_t __n);
int	memcmp(const void *__s1, const void *__s2, size_t __n);
void	*memcpy(void *__dest, const void *__src, size_t __n);
void	*memmove(void *__dest, const void *__src, size_t __n);
void	*memset(void *__s, int __c, size_t __n);
char	*strcat(char *__dest, const char *__src);
char	*strchr(const char *__s, int __c);
int	strcmp(const char *__s1, const char *__s2);
int	strcoll(const char *__s1, const char *__s2);
char	*strcpy(char *__dest, const char *__src);
size_t	strcspn(const char *__s, const char *__reject);
char	*strdup(const char *__s);
char	*strerror(int __errnum);
size_t	strlen(const char *__s);
char	*strncat(char *__dest, const char *__src, size_t __n);
int	strncmp(const char *__s1, const char *__s2, size_t __n);
char	*strncpy(char *__dest, const char *__src, size_t __n);
char	*strpbrk(const char *__s, const char *);
char	*strrchr(const char *__s, int);
size_t	strspn(const char *__s, const char *__accept);
char	*strstr(const char *__haystack, const char *__needle);
char	*strtok(char *__s, const char *__delim);
size_t	strxfrm(char *__dest, const char *__src, size_t __n);

/* Other common BSD functions */
char *strsep(char **__stringp, const char *__delim);

int	ffsl(long);

#endif /* _STRING_H_ */
