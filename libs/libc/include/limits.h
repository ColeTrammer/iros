#ifndef _LIMITS_H
#define _LIMITS_H 1

#define CHAR_BIT 8

#define INT_MAX 2147483647
#define INT_MIN -2147483648

#define LONG_MAX 9223372036854775807L
#define LONG_MIN -9223372036854775807L

#define LLONG_MAX ((long long) LONG_MAX)
#define LLONG_MIN ((long long) LONG_MIN)

#define ULLONG_MAX 18446744073709551615ULL

#ifndef __is_kernel

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

#define PAGESIZE                      0x1000
#define PAGE_SIZE                     PAGESIZE
#define PTHREAD_DESTRUCTOR_ITERATIONS 255
#define PTHREAD_KEYS_MAX              _POSIX_THREAD_KEYS_MAX
#define PTHREAD_STACK_MIN             (PAGE_SIZE * 4)
#define PTHREAD_THREADS_MAX           _POSIX_THREAD_THREADS_MAX

#endif /* __is_kernel */

#endif /* _LIMITS_H */