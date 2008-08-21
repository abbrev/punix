#ifndef _FNMATCH_H_
#define _FNMATCH_H_

/* values for 'flags' argument of fnmatch() */
#define FNM_PATHNAME	0x01	/* slash in string only matches slash in pattern */
#define FNM_PERIOD	0x02	/* leading period in string must be exactly matched by period in pattern */
#define FNM_NOESCAPE	0x04	/* disable backslash escaping */

/* return value for fnmatch() */
#define FNM_NOMATCH	1	/* the string does not match the specified pattern */
#define FNM_NOSYS	-1	/* reserved (obsolete) */

int	fnmatch(const char *__pattern, const char *__string, int __flags);

#endif
