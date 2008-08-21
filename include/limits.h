#ifndef _LIMITS_H_
#define _LIMITS_H_

/* $Id: limits.h,v 1.3 2007/12/12 23:30:48 fredfoobar Exp $ */
/* This should mostly conform to POSIX */

/*
 * Minimum acceptable maximum values
 */

#define _POSIX_AIO_LISTIO_MAX		2
#define _POSIX_AIO_MAX			1
#define _POSIX_ARG_MAX			4096
#define _POSIX_CHILD_MAX		25
#define _POSIX_DELAYTIMER_MAX		32
#define _POSIX_HOST_NAME_MAX		255
#define _POSIX_LINK_MAX			8
#define _POSIX_LOGIN_NAME_MAX		9
#define _POSIX_MAX_CANON		255
#define _POSIX_MAX_INPUT		255
#define _POSIX_MQ_OPEN_MAX		8
#define _POSIX_MQ_PRIO_MAX		32
#define _POSIX_NAME_MAX			14
#define _POSIX_NGROUPS_MAX		8
#define _POSIX_OPEN_MAX			20
#define _POSIX_PATH_MAX			256
#define _POSIX_PIPE_BUF			512
#define _POSIX_RE_DUP_MAX		255
#define _POSIX_RTSIG_MAX		8
#define _POSIX_SEM_NSEMS_MAX		256
#define _POSIX_SEM_VALUE_MAX		32767
#define _POSIX_SIGQUEUE_MAX		32
#define _POSIX_SSIZE_MAX		32767
#define _POSIX_STREAM_MAX		8
#define _POSIX_SS_REPL_MAX		4
#define _POSIX_SYMLINK_MAX		255
#define _POSIX_SYMLOOP_MAX		8
#define _POSIX_TRACE_EVENT_NAME_MAX	30
#define _POSIX_TRACE_NAME_MAX		8
#define _POSIX_TRACE_SYS_MAX		8
#define _POSIX_TRACE_USER_EVENT_MAX	32
#define _POSIX_TTY_NAME_MAX		9
#define _POSIX_TZNAME_MAX		6


#define _POSIX2_BC_BASE_MAX		99
#define _POSIX2_BC_DIM_MAX		2048
#define _POSIX2_BC_SCALE_MAX		99
#define _POSIX2_BC_STRING_MAX		1000
#define _POSIX2_CHARCLASS_NAME_MAX	14
#define _POSIX2_COLL_WEIGHTS_MAX	2
#define _POSIX2_EXPR_NEST_MAX		32
#define _POSIX2_LINE_MAX		2048
#define _POSIX2_RE_DUP_MAX		255
/*
#define _XOPEN_IOV_MAX			16
#define _XOPEN_NAME_MAX			255
             [XSI] [Option Start]
             Maximum number of bytes in a filename (not including the
             terminating null).
             Value: 255 [Option End]
#define _XOPEN_PATH_MAX			1024
             [XSI] [Option Start]
             Maximum number of bytes in a pathname.
             Value: 1024 [Option End]
*/

/*
 * Maximum values
 */

#define AIO_LISTIO_MAX		_POSIX_AIO_LISTIO_MAX
#define AIO_MAX			_POSIX_AIO_MAX
#define AIO_PRIO_DELTA_MAX	0
#define ARG_MAX			_POSIX_ARG_MAX
#define ATEXIT_MAX		32
#define CHILD_MAX		_POSIX_CHILD_MAX
#define DELAYTIMER_MAX		_POSIX_DELAYTIMER_MAX
#define HOST_NAME_MAX		_POSIX_HOST_NAME_MAX
#define IOV_MAX			_XOPEN_IOV_MAX
#define LOGIN_NAME_MAX		_POSIX_LOGIN_NAME_MAX
#define OPEN_MAX		_POSIX_OPEN_MAX
#define PAGESIZE		1
#define PAGE_SIZE		PAGESIZE
#define RE_DUP_MAX		_POSIX2_RE_DUP_MAX
#define RTSIG_MAX		_POSIX_RTSIG_MAX
#define SEM_NSEMS_MAX		_POSIX_SEM_NSEMS_MAX
#define SEM_VALUE_MAX		_POSIX_SEM_VALUE_MAX
#define SIGQUEUE_MAX		_POSIX_SIGQUEUE_MAX
#define SS_REPL_MAX		_POSIX_SS_REPL_MAX
#define STREAM_MAX		_POSIX_STREAM_MAX
#define SYMLOOP_MAX		_POSIX_SYMLOOP_MAX
#define TIMER_MAX		_POSIX_TIMER_MAX
#define TRACE_EVENT_NAME_MAX	_POSIX_TRACE_EVENT_NAME_MAX
#define TRACE_NAME_MAX		_POSIX_TRACE_NAME_MAX
#define TRACE_SYS_MAX		_POSIX_TRACE_SYS_MAX
#define TRACE_USER_EVENT_MAX	_POSIX_TRACE_USER_EVENT_MAX
#define TTY_NAME_MAX		_POSIX_TTY_NAME_MAX
#define TZNAME_MAX		_POSIX_TZNAME_MAX

#define FILESIZEBITS	32
#define LINK_MAX	_POSIX_LINK_MAX
#define MAX_CANON	_POSIX_MAX_CANON
#define MAX_INPUT	_POSIX_MAX_INPUT
#define NAME_MAX	_POSIX_NAME_MAX
#define PATH_MAX	_POSIX_PATH_MAX
#define PIPE_BUF	_POSIX_PIPE_BUF
/*
#define POSIX_ALLOC_SIZE_MIN	
             [ADV] [Option Start]
             Minimum number of bytes of storage actually allocated for any
             portion of a file.
             Minimum Acceptable Value: Not specified. [Option End]
#define POSIX_REC_INCR_XFER_SIZE	
             [ADV] [Option Start]
             Recommended increment for file transfer sizes between the
             {POSIX_REC_MIN_XFER_SIZE} and {POSIX_REC_MAX_XFER_SIZE} values.
             Minimum Acceptable Value: Not specified. [Option End]
#define POSIX_REC_MAX_XFER_SIZE	
             [ADV] [Option Start]
             Maximum recommended file transfer size.
             Minimum Acceptable Value: Not specified. [Option End]
#define POSIX_REC_MIN_XFER_SIZE	
             [ADV] [Option Start]
             Minimum recommended file transfer size.
             Minimum Acceptable Value: Not specified. [Option End]
#define POSIX_REC_XFER_ALIGN	
             [ADV] [Option Start]
             Recommended file transfer buffer alignment.
             Minimum Acceptable Value: Not specified. [Option End]
*/
#define SYMLINK_MAX	_POSIX_SYMLINK_MAX

#define BC_BASE_MAX		_POSIX2_BC_BASE_MAX
#define BC_DIM_MAX		_POSIX2_BC_DIM_MAX
#define BC_SCALE_MAX		_POSIX2_BC_SCALE_MAX
#define BC_STRING_MAX		_POSIX2_BC_STRING_MAX
#define CHARCLASS_NAME_MAX	_POSIX2_CHARCLASS_NAME_MAX
#define COLL_WEIGHTS_MAX	_POSIX2_COLL_WEIGHTS_MAX
#define EXPR_NEST_MAX		_POSIX2_EXPR_NEST_MAX
#define LINE_MAX		_POSIX2_LINE_MAX
#define NGROUPS_MAX		_POSIX_NGROUPS_MAX
#define RE_DUP_MAX		_POSIX2_RE_DUP_MAX

/* Numerical */
/* char */
#define CHAR_BIT	8
#define SCHAR_MAX	127
#define SCHAR_MIN	(-128)
#define UCHAR_MAX	255
#define CHAR_MAX	127
#define CHAR_MIN	(-128)

/* short */
#define SHRT_MAX	32767
#define SHRT_MIN	(-32768)
#define USHRT_MAX	65535

/* int */
/*
#define WORD_BIT	16
             [XSI] [Option Start]
             Number of bits in a type int.
             Minimum Acceptable Value: 32 [Option End]
*/
#define INT_MAX		32767
#define INT_MIN		(-32768)
#define UINT_MAX	65535

/* long */
#define LONG_BIT	32
#define LONG_MAX	2147483647L
#define LONG_MIN	(-2147483647L)
#define ULONG_MAX	4294967295UL

/* long long */
#define LLONG_MIN	(-9223372036854775808LL)
#define LLONG_MAX	9223372036854775807LL
#define ULLONG_MAX	18446744073709551615ULL

#define MB_LEN_MAX	1
#define SSIZE_MAX	LONG_MAX

#if 0
/* Other */
#define NL_ARGMAX	9
#define NL_LANGMAX	14
#define NL_MSGMAX	32767
/*
#define NL_NMAX	
             [XSI] [Option Start]
             Maximum number of bytes in an N-to-1 collation mapping.
             Minimum Acceptable Value: No guaranteed value across all
             conforming implementations. [Option End]
*/

#define NL_SETMAX	255
#define NL_TEXTMAX	_POSIX2_LINE_MAX
#endif

#define NZERO		20

#endif
