#ifndef _TICKS_H_
#define _TICKS_H_

#define time_after(a, b) ((long)(b) - (long)(a) < 0)
#define time_before(a, b) time_after(b, a)

#define time_after_eq(a, b) ((long)(a) - (long)(b) >= 0)
#define time_before_eq(a, b) time_after_eq(b, a)

#endif
