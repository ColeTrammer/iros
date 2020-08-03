#ifndef _LIMITS_H
#define _LIMITS_H 1

#define CHAR_BIT  8
#define SCHAR_MAX 127
#define SCHAR_MIN (-SCHAR_MAX - 1)
#define UCHAR_MAX 255U
#define CHAR_MIN  SCHAR_MIN
#define CHAR_MAX  SCHAR_MAX

#define SHRT_MAX  32767
#define SHRT_MIN  (-SHRT_MAX - 1)
#define USHRT_MAX 65535U

#define WORD_BIT 32
#define INT_MAX  2147483647
#define INT_MIN  (-INT_MAX - 1)
#define UINT_MAX 4294967295U

#define LONG_BIT  64
#define LONG_MAX  9223372036854775807L
#define LONG_MIN  (-LONG_MAX - 1)
#define ULONG_MAX 18446744073709551615UL

#define LLONG_MAX  9223372036854775807LL
#define LLONG_MIN  (-LLONG_MAX - 1)
#define ULLONG_MAX 18446744073709551615ULL

#define MB_LEN_MAX 1

/* Kernel doesn't get to see POSIX things */
#ifndef __is_kernel

#define SSIZE_MAX LONG_MAX

#define _POSIX_CLOCKRES_MIN 20000000

#define _POSIX_AIO_LISTIO_MAX               2
#define _POSIX_AIO_MAX                      1
#define _POSIX_ARG_MAX                      4096
#define _POSIX_CHILD_MAX                    25
#define _POSIX_DELAYTIMER_MAX               32
#define _POXIX_HOST_NAME_MAX                255
#define _POSIX_LINK_MAX                     8
#define _POSIX_LOGIN_NAME_MAX               9
#define _POSIX_MAX_CANON                    255
#define _POSIX_MAX_INPUT                    255
#define _POSIX_MQ_OPEN_MAX                  8
#define _POSIX_MQ_PRIO_MAX                  8
#define _POSIX_NAME_MAX                     14
#define _POSIX_NGROUPS_MAX                  8
#define _POSIX_OPEN_MAX                     20
#define _POSIX_PATH_MAX                     256
#define _POSIX_PIPE_BUF                     512
#define _POSIX_RE_DUP_MAX                   255
#define _POSIX_RTSIG_MAX                    8
#define _POSIX_SEM_NSEMS_MAX                256
#define _POSIX_SEM_VALUE_MAX                32767
#define _POSIX_SIGQUEUE_MAX                 32
#define _POSIX_SSIZE_MAX                    32376
#define _POSIX_SS_REPL_MAX                  4
#define _POSIX_STREAM_MAX                   8
#define _POSIX_SYMLINK_MAX                  255
#define _POSIX_SYMLOOP_MAX                  8
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS 4
#define _POSIX_THREAD_KEYS_MAX              128
#define _POSIX_THREAD_THREADS_MAX           64
#define _POSIX_TIMER_MAX                    32
#define _POSIX_TRACE_EVENT_NAME_MAX         30
#define _POSIX_TRACE_NAME_MAX               8
#define _POSIX_TRACE_USER_EVENT_MAX         32
#define _POSIX_TTY_NAME_MAX                 9
#define _POSIX_TZNAME_MAX                   6

#define _POSIX2_BC_BASE_MAX        99
#define _POSIX2_BC_DIM_MAX         2048
#define _POSIX2_BC_SCALE_MAX       99
#define _POSIX2_BC_STRING_MAX      1000
#define _POSIX2_CHARCLASS_NAME_MAX 14
#define _POSIX2_COLL_WEIGHTS_MAX   2
#define _POSIX2_EXPR_NEST_MAX      32
#define _POSIX2_LINE_MAX           2048
#define _POSIX2_RE_DUP_MAX         255

#define _XOPEN_IOV_MAX  32
#define _XOPEN_NAME_MAX 255
#define _XOPEN_PATH_MAX 1024

/* #undef AIO_LISTIO_MAX */
/* #undef AIO_MAX */
/* #undef AIO_PRIO_DELTA_MAX */
#define ARG_MAX 2 * 1024 * 1024
/* #undef ATEXIT_MAX */
/* #undef CHILD_MAX */
/* #undef DELAYTIMER_MAX */
/* #undef HOST_NAME_MAX */
#define IOV_MAX 1024
/* #undef LOGIN_NAME_MAX */
/* #undef MQ_OPEN_MAX */
/* #undef MQ_PRIO_MAX */
#define OPEN_MAX                      16
#define PAGESIZE                      4096
#define PAGE_SIZE                     PAGESIZE
#define PTHREAD_DESTRUCTOR_ITERATIONS 255
#define PTHREAD_KEYS_MAX              _POSIX_THREAD_KEYS_MAX
#define PTHREAD_STACK_MIN             (PAGE_SIZE * 4)
#define PTHREAD_THREADS_MAX           _POSIX_THREAD_THREADS_MAX
/* #undef RE_DUP_MAX */
#define RTSIG_MAX 30
/* #undef SEM_NSEMS_MAX */
/* #undef SEM_VALUE_MAX */
/* #undef SIGQUEUE_MAX */
/* #undef SS_REPL_MAX */
#define STREAM_MAX  OPEN_MAX
#define SYMLOOP_MAX 25
/* #undef TIMER_MAX */
/* #undef TRACE_EVENT_NAME_MAX */
/* #undef TRACE_NAME_MAX */
/* #undef TRACE_SYS_MAX */
/* #undef TRACE_USER_EVENT_MAX */
/* #undef TTY_NAME_MAX */
/* #undef TZNAME_MAX */

#define FILESIZEBITS 32
#define LINK_MAX     255
/* #undef MAX_CANON */
/* #undef MAX_INPUT */
#define NAME_MAX 255
#define PATH_MAX 4096
#define PIPE_BUF 4096
/* #undef POSIX_ALLOC_SIZE_MIN */
/* #undef POSIX_REC_INCR_XFER_SIZE */
/* #undef POSIX_REC_MAX_XFER_SIZE */
/* #undef POSIX_REC_MIN_XFER_SIZE */
/* #undef POSIX_REC_XFER_ALIGN */
/* #undef SYMLINK_MAX */

/* #undef BC_BASE_MAX */
/* #undef BC_DIM_MAX */
/* #undef BC_SCALE_MAX */
/* #undef BC_STRING_MAX */
/* #undef CHARCLASS_NAME_MAX */
/* #undef COLL_WEIGHTS_MAX */
/* #undef EXPR_NEST_MAX */
/* #undef LINE_MAX */
/* #undef NGROUPS_MAX */
/* #undef RE_DUP_MAX */

#define NL_ARGMAX  9
#define NL_LANGMAX 14
#define NL_MSGMAX  32767
#define NL_NMAX    255
#define NL_SETMAX  255
#define NL_TEXTMAX _POSIX2_LINE_MAX
#define NZERO      20

#endif /* __is_kernel */

#endif /* _LIMITS_H */
