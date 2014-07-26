#ifndef _REGEX_H_
#define _REGEX_H_

/* mostly POSIX compliant */

#include <sys/types.h>

struct regex_t {
	size_t	re_nsub;	/* number of parenthesized subexpressions */
};

typedef ssize_t	regoff_t;

struct regmatch_t {
	regoff_t	rm_so;	/* offset from start of string to start of substring */
	regoff_t	rm_eo;	/* offset from start of string of the first character after the end of substring */
};

/* values for cflags parameter to regcomp() */
#define REG_EXTENDED	0x01	/* use Extended Regular Expressions */
#define REG_ICASE	0x02	/* ignore case in match */
#define REG_NOSUB	0x04	/* report only success or fail in regexec() */
#define REG_NEWLINE	0x08	/* change the handling of <newline> */

/* values for eflags parameter to regexec() */
#define REG_NOTBOL	0x01	/* '^' does not match the beginning of string */
#define REG_NOTEOL	0x02	/* '$' does not match the end of string */

/* error return values */
#define REG_NOMATCH	1	/* regexec() failed to match */
#define REG_BADPAT	2	/* invalid regular expression */
#define REG_ECOLLATE	3	/* invalid collating element referenced */
#define REG_ECTYPE	4	/* invalid character class type referenced */
#define REG_EESCAPE	5	/* trailing '\' in pattern */
#define REG_ESUBREG	6	/* number in \digit invalid or in error */
#define REG_EBRACK	7	/* "[]" imbalance */
#define REG_EPAREN	8	/* "\(\)" or "()" imbalance */
#define REG_EBRACE	9	/* "\{\}" imbalance */
#define REG_BADBR	10	/* content of "\{\}" invalid */
#define REG_ERANGE	11	/* invalid endpoint in range expression */
#define REG_ESPACE	12	/* out of memory */
#define REG_BADRPT	13	/* '?', '*', or '+' not preceded by valid regular expression */
#define REG_ENOSYS	-1	/* obsolete (reserved) */

int	regcomp(regex_t *__preg, const char *__regex, int __cflags);
size_t	regerror(int __errcode, const regex_t *__preg, char *__errbuf,
		size_t __errbuf_size);
int	regexec(const regex_t *__preg, const char *__string, size_t __nmatch,
		regmatch_t __pmatch[], int __eflags);
void	regfree(regex_t *__preg);

#endif
