#ifndef _STDBOOL_H_
#define _STDBOOL_H_

/* POSIX states that 'bool' must expand to '_Bool',
 * but it does not define '_Bool' */
#define bool	_Bool

#define false	0
#define true	1

#define __bool_true_false_are_defined	1

#endif
