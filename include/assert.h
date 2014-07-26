/* $Id$ */

/*
 * this is almost POSIX compliant
 * The only non-compliant part (that I can see) is that assert doesn't print the
 * function in which the assert fails, though it's not really a big problem.
 */

#undef assert

#ifdef NDEBUG

/* do nothing */
#define assert(expr)		((void)0)

#else /* NDEBUG */

extern void	__assert(const char *__assertion, const char *__file,
		unsigned __line);

#define assert(expr)	\
	((void)((expr) ? 0 : __assert(#expr, __FILE__, __LINE__)))

#endif /* NDEBUG */
