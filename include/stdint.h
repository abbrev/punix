#ifndef _STDINT_H_
#define _STDINT_H_

/* $Id: stdint.h,v 1.1 2007/05/03 08:59:58 fredfoobar Exp $ */

#include <limits.h>

/* more or less POSIX compliant */

typedef signed char		int8_t;
typedef signed short		int16_t;
typedef signed long		int32_t;
typedef signed long long	int64_t;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;

typedef signed char		int_least8_t;
typedef signed short		int_least16_t;
typedef signed long		int_least32_t;
typedef signed long long	int_least64_t;

typedef unsigned char		uint_least8_t;
typedef unsigned short		uint_least16_t;
typedef unsigned long		uint_least32_t;
typedef unsigned long long	uint_least64_t;

typedef signed char		int_fast8_t; /* XXX: is signed short faster? */
typedef signed short		int_fast16_t;
typedef signed long		int_fast32_t;
typedef signed long long	int_fast64_t;

typedef unsigned char		uint_fast8_t; /* XXX: is unsigned short faster? */
typedef unsigned short		uint_fast16_t;
typedef unsigned long		uint_fast32_t;
typedef unsigned long long	uint_fast64_t;

typedef signed long		intptr_t;

typedef unsigned long		uintptr_t;

typedef signed long long	intmax_t;

typedef unsigned long long	uintmax_t;

#define INT8_MIN	(-128)
#define INT8_MAX	127
#define UINT8_MAX	255

#define INT16_MIN	(-32768)
#define INT16_MAX	32767
#define UINT16_MAX	65535

#define INT32_MIN	(-2147483648L)
#define INT32_MAX	2147483647L
#define UINT32_MAX	4294967296UL

#define INT64_MIN	(-9223372036854775808LL)
#define INT64_MAX	9223372036854775807LL
#define UINT64_MAX	18446744073709551616ULL

#define INT_LEAST8_MIN		(-128)
#define INT_LEAST8_MAX		127
#define UINT_LEAST8_MAX		255

#define INT_LEAST16_MIN		(-32768)
#define INT_LEAST16_MAX		32767
#define UINT_LEAST16_MAX	65535

#define INT_LEAST32_MIN		(-2147483648L)
#define INT_LEAST32_MAX		2147483647L
#define UINT_LEAST32_MAX	4294967296UL

#define INT_LEAST64_MIN		(-9223372036854775808LL)
#define INT_LEAST64_MAX		9223372036854775807LL
#define UINT_LEAST64_MAX	18446744073709551616ULL

#define INT_FAST8_MIN	(-128)
#define INT_FAST8_MAX	127
#define UINT_FAST8_MAX	255

#define INT_FAST16_MIN	(-32768)
#define INT_FAST16_MAX	32767
#define UINT_FAST16_MAX	65535

#define INT_FAST32_MIN	(-2147483648L)
#define INT_FAST32_MAX	2147483647L
#define UINT_FAST32_MAX	4294967296UL

#define INT_FAST64_MIN	(-9223372036854775808LL)
#define INT_FAST64_MAX	9223372036854775807LL
#define UINT_FAST64_MAX	18446744073709551616ULL

#define INTPTR_MIN	(-2147483648L)
#define INTPTR_MAX	2147483647L
#define UINTPTR_MAX	4294967296UL

#define INTMAX_MIN	(-9223372036854775808LL)
#define INTMAX_MAX	9223372036854775807LL
#define UINTMAX_MAX	18446744073709551616ULL


#define PTRDIFF_MIN	(-2147483648L)
#define PTRDIFF_MAX	2147483647L

/* see <signal.h> */
/*
#define SIG_ATOMIC_MIN	
#define SIG_ATOMIC_MAX	
*/

#define SIZE_MAX	65535

/* see <stddef.h> */
#define WCHAR_MIN	CHAR_MIN
#define WCHAR_MAX	CHAR_MAX

/* see <wchar.h> */
/*
#define WINT_MIN	
#define WINT_MAX	
*/

#define INT8_C(n)	n
#define INT16_C(n)	n
#define INT32_C(n)	n##L
#define INT64_C(n)	n##LL
#define UINT8_C(n)	n##U
#define UINT16_C(n)	n##U
#define UINT32_C(n)	n##UL
#define UINT64_C(n)	n##ULL
#define INTMAX_C(n)	n##LL
#define UINTMAX_C(n)	n##ULL

#endif
