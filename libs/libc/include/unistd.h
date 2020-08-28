#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <bits/gid_t.h>
#include <bits/null.h>
#include <bits/off_t.h>
#include <bits/pid_t.h>
#include <bits/size_t.h>
#include <bits/ssize_t.h>
#include <bits/uid_t.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */

#define _POSIX_VERSION  200809L
#define _POSIX2_VERSION 200809L
#define _XOPEN_VERSION  700

#define _POSIX_ADVISORY_INFO              -1
#define _POSIX_ASYNCHRONOUS_IO            -1
#define _POSIX_BARRIERS                   -1
#define _POSIX_CHOWN_RESTRICTED           1
#define _POSIX_CLOCK_SELECTION            200809L
#define _POSIX_CPUTIME                    200809L
#define _POSIX_FSYNC                      200809L
#define _POSIX_IPV6                       -1
#define _POSIX_JOB_CONTROL                1
#define _POSIX_MAPPED_FILES               200809L
#define _POSIX_MEMLOCK                    -1
#define _POSIX_MEMLOCK_RANGE              -1
#define _POSIX_MEMORY_PROTECTION          200809L
#define _POSIX_MESSAGE_PASSING            -1
#define _POSIX_MONOTONIC_CLOCK            200809L
#define _POSIX_NO_TRUNC                   1
#define _POSIX_PRIORITIZED_IO             -1
#define _POSIX_PRIORITY_SCHEDULING        -1
#define _POSIX_RAW_SOCKETS                200809L
#define _POSIX_READER_WRITER_LOCKS        -1
#define _POSIX_REALTIME_SIGNALS           200809L
#define _POSIX_REGEXP                     1
#define _POSIX_SAVED_IDS                  1
#define _POSIX_SEMAPHORES                 200809L
#define _POSIX_SHARED_MEMORY_OBJECTS      200809L
#define _POSIX_SHELL                      200809L
#define _POSIX_SPAWN                      -1
#define _POSIX_SPINLOCKS                  200809L
#define _POSIX_SPORADIC_SERVER            -1
#define _POSIX_SYNCHRONIZED_IO            -1
#define _POSIX_THREAD_ATTR_STACKADDR      200809L
#define _POSIX_THREAD_ATTR_STACKSIZE      200809L
#define _POSIX_THREAD_CPUTIME             200809L
#define _POSIX_THREAD_PRIO_INHERIT        -1
#define _POSIX_THREAD_PRIO_PROTECT        -1
#define _POSIX_THREAD_PRIORITY_SCHEDULING -1
#define _POSIX_THREAD_PROCESS_SHARED      200809L
#define _POSIX_THREAD_ROBUST_PRIO_INHERIT -1
#define _POSIX_THREAD_ROBUST_PRIO_PROTECT -1
#define _POSIX_THREAD_SAFE_FUNCTIONS      200809L
#define _POSIX_THREAD_SPORADIC_SERVER     -1
#define _POSIX_THREADS                    200809L
#define _POSIX_TIMEOUTS                   200809L
#define _POSIX_TIMERS                     200809L
#define _POSIX_TRACE                      -1
#define _POSIX_TRACE_EVENT_FILTER         -1
#define _POSIX_TRACE_INHERIT              -1
#define _POSIX_TRACE_LOG                  -1
#define _POSIX_TYPED_MEMORY_OBJECTS       -1
#define _POSIX_V6_ILP32_OFFBIG            -1
#define _POSIX_V6_ILP32_OFF32             -1
#define _POSIX_V6_LP64_OFF64              -1
#define _POSIX_V6_LPBIG_OFFBIG            1
#define _POSIX_V7_ILP32_OFF32             -1
#define _POSIX_V7_ILP32_OFFBIG            -1
#define _POSIX_V7_LP64_OFF64              1
#define _POSIX_V7_LPBIG_OFFBIG            -1
#define _POSIX2_C_BIND                    200809L
#define _POSIX2_C_DEV                     -1
#define _POSIX2_CHAR_TERM                 1
#define _POSIX2_FORT_DEV                  -1
#define _POSIX2_FORT_RUN                  -1
#define _POSIX2_LOCALEDEF                 -1
#define _POSIX2_PBS                       -1
#define _POSIX2_PBS_ACCOUNTING            -1
#define _POSIX2_PBS_CHECKPOINT            -1
#define _POSIX2_PBS_LOCATE                -1
#define _POSIX2_PBS_MESSAGE               -1
#define _POSIX2_PBS_TRACK                 -1
#define _POSIX2_SW_DEV                    -1
#define _POSIX2_UPE                       -1
#define _XOPEN_CRYPT                      -1
#define _XOPEN_ENH_I18N                   -1
#define _XOPEN_REALTIME                   -1
#define _XOPEN_REALTIME_THREADS           -1
#define _XOPEN_SHM                        -1
#define _XOPEN_STREAMS                    -1
#define _XOPEN_UNIX                       1
#define _XOPEN_UUCP                       -1

#define _POSIX_ASYNC_IO -1
#define _POSIX_PRIO_IO  -1
#define _POSIX_SYNC_IO  -1
/* #undef _POSIX_TIMESTAMP_RESOLUTION */
/* #undef _POSIX2_SYMLINKS */

#define F_OK 0
#define W_OK 1
#define R_OK 2
#define X_OK 4

enum {
    _CS_PATH,
#define _CS_PATH _CS_PATH
    _CS_POSIX_V7_ILP32_OFF32_CFLAGS,
#define _CS_POSIX_V7_ILP32_OFF32_CFLAGS _CS_POSIX_V7_ILP32_OFF32_CFLAGS
    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,
#define _CS_POSIX_V7_ILP32_OFF32_LDFLAGS _CS_POSIX_V7_ILP32_OFF32_LDFLAGS
    _CS_POSIX_V7_ILP32_OFF32_LIBS,
#define _CS_POSIX_V7_ILP32_OFF32_LIBS _CS_POSIX_V7_ILP32_OFF32_LIBS
    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,
#define _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS
    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,
#define _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS
    _CS_POSIX_V7_ILP32_OFFBIG_LIBS,
#define _CS_POSIX_V7_ILP32_OFFBIG_LIBS _CS_POSIX_V7_ILP32_OFFBIG_LIBS
    _CS_POSIX_V7_LP64_OFF64_CFLAGS,
#define _CS_POSIX_V7_LP64_OFF64_CFLAGS _CS_POSIX_V7_LP64_OFF64_CFLAGS
    _CS_POSIX_V7_LP64_OFF64_LDFLAGS,
#define _CS_POSIX_V7_LP64_OFF64_LDFLAGS _CS_POSIX_V7_LP64_OFF64_LDFLAGS
    _CS_POSIX_V7_LP64_OFF64_LIBS,
#define _CS_POSIX_V7_LP64_OFF64_LIBS _CS_POSIX_V7_LP64_OFF64_LIBS
    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,
#define _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS
    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,
#define _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS
    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,
#define _CS_POSIX_V7_LPBIG_OFFBIG_LIBS _CS_POSIX_V7_LPBIG_OFFBIG_LIBS
    _CS_POSIX_V7_THREADS_CFLAGS,
#define _CS_POSIX_V7_THREADS_CFLAGS _CS_POSIX_V7_THREADS_CFLAGS
    _CS_POSIX_V7_THREADS_LDFLAGS,
#define _CS_POSIX_V7_THREADS_LDFLAGS _CS_POSIX_V7_THREADS_LDFLAGS
    _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS,
#define _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS
    _CS_V7_ENV,
#define _CS_V7_ENV _CS_V7_ENV
    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,
#define _CS_POSIX_V6_ILP32_OFF32_CFLAGS _CS_POSIX_V6_ILP32_OFF32_CFLAGS
    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,
#define _CS_POSIX_V6_ILP32_OFF32_LDFLAGS _CS_POSIX_V6_ILP32_OFF32_LDFLAGS
    _CS_POSIX_V6_ILP32_OFF32_LIBS,
#define _CS_POSIX_V6_ILP32_OFF32_LIBS _CS_POSIX_V6_ILP32_OFF32_LIBS
    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,
#define _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS
    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,
#define _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS
    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,
#define _CS_POSIX_V6_ILP32_OFFBIG_LIBS _CS_POSIX_V6_ILP32_OFFBIG_LIBS
    _CS_POSIX_V6_LP64_OFF64_CFLAGS,
#define _CS_POSIX_V6_LP64_OFF64_CFLAGS _CS_POSIX_V6_LP64_OFF64_CFLAGS
    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,
#define _CS_POSIX_V6_LP64_OFF64_LDFLAGS _CS_POSIX_V6_LP64_OFF64_LDFLAGS
    _CS_POSIX_V6_LP64_OFF64_LIBS,
#define _CS_POSIX_V6_LP64_OFF64_LIBS _CS_POSIX_V6_LP64_OFF64_LIBS
    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,
#define _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS
    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,
#define _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS
    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,
#define _CS_POSIX_V6_LPBIG_OFFBIG_LIBS _CS_POSIX_V6_LPBIG_OFFBIG_LIBS
    _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS,
#define _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS
    _CS_V6_ENV
#define _CS_V6_ENV _CS_V6_ENV
};

/* #define F_LOCK 1 */
/* #define F_TEST 2 */
/* #define F_TLOCK 3 */
/* #define F_ULOCK 4 */

enum {
    _PC_2_SYMLINKS,
#define _PC_2_SYMLINKS _PC_2_SYMLINKS
    _PC_ALLOC_SIZE_MIN,
#define _PC_ALLOC_SIZE_MIN _PC_ALLOC_SIZE_MIN
    _PC_ASYNC_IO,
#define _PC_ASYNC_IO _PC_ASYNC_IO
    _PC_CHOWN_RESTRICTED,
#define _PC_CHOWN_RESTRICTED _PC_CHOWN_RESTRICTED
    _PC_FILESIZEBITS,
#define _PC_FILESIZEBITS _PC_FILESIZEBITS
    _PC_LINK_MAX,
#define _PC_LINK_MAX _PC_LINK_MAX
    _PC_MAX_CANON,
#define _PC_MAX_CANON _PC_MAX_CANON
    _PC_MAX_INPUT,
#define _PC_MAX_INPUT _PC_MAX_INPUT
    _PC_NAME_MAX,
#define _PC_NAME_MAX _PC_NAME_MAX
    _PC_NO_TRUNC,
#define _PC_NO_TRUNC _PC_NO_TRUNC
    _PC_PATH_MAX,
#define _PC_PATH_MAX _PC_PATH_MAX
    _PC_PIPE_BUF,
#define _PC_PIPE_BUF _PC_PIPE_BUF
    _PC_PRIO_IO,
#define _PC_PRIO_IO _PC_PRIO_IO
    _PC_REC_INCR_XFER_SIZE,
#define _PC_REC_INCR_XFER_SIZE _PC_REC_INCR_XFER_SIZE
    _PC_REC_MAX_XFER_SIZE,
#define _PC_REC_MAX_XFER_SIZE _PC_REC_MAX_XFER_SIZE
    _PC_REC_MIN_XFER_SIZE,
#define _PC_REC_MIN_XFER_SIZE _PC_REC_MIN_XFER_SIZE
    _PC_REC_XFER_ALIGN,
#define _PC_REC_XFER_ALIGN _PC_REC_XFER_ALIGN
    _PC_SYMLINK_MAX,
#define _PC_SYMLINK_MAX _PC_SYMLINK_MAX
    _PC_SYNC_IO,
#define _PC_SYNC_IO _PC_SYNC_IO
    _PC_TIMESTAMP_RESOLUTION,
#define _PC_TIMESTAMP_RESOLUTION _PC_TIMESTAMP_RESOLUTION
    _PC_VDISABLE
#define _PC_VDISABLE _PC_VDISABLE
};

enum {
    _SC_2_C_BIND,
#define _SC_2_C_BIND _SC_2_C_BIND
    _SC_2_C_DEV,
#define _SC_2_C_DEV _SC_2_C_DEV
    _SC_2_CHAR_TERM,
#define _SC_2_CHAR_TERM _SC_2_CHAR_TERM
    _SC_2_FORT_DEV,
#define _SC_2_FORT_DEV _SC_2_FORT_DEV
    _SC_2_FORT_RUN,
#define _SC_2_FORT_RUN _SC_2_FORT_RUN
    _SC_2_LOCALEDEF,
#define _SC_2_LOCALEDEF _SC_2_LOCALEDEF
    _SC_2_PBS,
#define _SC_2_PBS _SC_2_PBS
    _SC_2_PBS_ACCOUNTING,
#define _SC_2_PBS_ACCOUNTING _SC_2_PBS_ACCOUNTING
    _SC_2_PBS_CHECKPOINT,
#define _SC_2_PBS_CHECKPOINT _SC_2_PBS_CHECKPOINT
    _SC_2_PBS_LOCATE,
#define _SC_2_PBS_LOCATE _SC_2_PBS_LOCATE
    _SC_2_PBS_MESSAGE,
#define _SC_2_PBS_MESSAGE _SC_2_PBS_MESSAGE
    _SC_2_PBS_TRACK,
#define _SC_2_PBS_TRACK _SC_2_PBS_TRACK
    _SC_2_SW_DEV,
#define _SC_2_SW_DEV _SC_2_SW_DEV
    _SC_2_UPE,
#define _SC_2_UPE _SC_2_UPE
    _SC_2_VERSION,
#define _SC_2_VERSION _SC_2_VERSION
    _SC_ADVISORY_INFO,
#define _SC_ADVISORY_INFO _SC_ADVISORY_INFO
    _SC_AIO_LISTIO_MAX,
#define _SC_AIO_LISTIO_MAX _SC_AIO_LISTIO_MAX
    _SC_AIO_MAX,
#define _SC_AIO_MAX _SC_AIO_MAX
    _SC_AIO_PRIO_DELTA_MAX,
#define _SC_AIO_PRIO_DELTA_MAX _SC_AIO_PRIO_DELTA_MAX
    _SC_ARG_MAX,
#define _SC_ARG_MAX _SC_ARG_MAX
    _SC_ASYNCHRONOUS_IO,
#define _SC_ASYNCHRONOUS_IO _SC_ASYNCHRONOUS_IO
    _SC_ATEXIT_MAX,
#define _SC_ATEXIT_MAX _SC_ATEXIT_MAX
    _SC_BARRIERS,
#define _SC_BARRIERS _SC_BARRIERS
    _SC_BC_BASE_MAX,
#define _SC_BC_BASE_MAX _SC_BC_BASE_MAX
    _SC_BC_DIM_MAX,
#define _SC_BC_DIM_MAX _SC_BC_DIM_MAX
    _SC_BC_SCALE_MAX,
#define _SC_BC_SCALE_MAX _SC_BC_SCALE_MAX
    _SC_BC_STRING_MAX,
#define _SC_BC_STRING_MAX _SC_BC_STRING_MAX
    _SC_CHILD_MAX,
#define _SC_CHILD_MAX _SC_CHILD_MAX
    _SC_CLK_TCK,
#define _SC_CLK_TCK _SC_CLK_TCK
    _SC_CLOCK_SELECTION,
#define _SC_CLOCK_SELECTION _SC_CLOCK_SELECTION
    _SC_COLL_WEIGHTS_MAX,
#define _SC_COLL_WEIGHTS_MAX _SC_COLL_WEIGHTS_MAX
    _SC_CPUTIME,
#define _SC_CPUTIME _SC_CPUTIME
    _SC_DELAYTIMER_MAX,
#define _SC_DELAYTIMER_MAX _SC_DELAYTIMER_MAX
    _SC_EXPR_NEST_MAX,
#define _SC_EXPR_NEST_MAX _SC_EXPR_NEST_MAX
    _SC_FSYNC,
#define _SC_FSYNC _SC_FSYNC
    _SC_GETGR_R_SIZE_MAX,
#define _SC_GETGR_R_SIZE_MAX _SC_GETGR_R_SIZE_MAX
    _SC_GETPW_R_SIZE_MAX,
#define _SC_GETPW_R_SIZE_MAX _SC_GETPW_R_SIZE_MAX
    _SC_HOST_NAME_MAX,
#define _SC_HOST_NAME_MAX _SC_HOST_NAME_MAX
    _SC_IOV_MAX,
#define _SC_IOV_MAX _SC_IOV_MAX
    _SC_IPV6,
#define _SC_IPV6 _SC_IPV6
    _SC_JOB_CONTROL,
#define _SC_JOB_CONTROL _SC_JOB_CONTROL
    _SC_LINE_MAX,
#define _SC_LINE_MAX _SC_LINE_MAX
    _SC_LOGIN_NAME_MAX,
#define _SC_LOGIN_NAME_MAX _SC_LOGIN_NAME_MAX
    _SC_MAPPED_FILES,
#define _SC_MAPPED_FILES _SC_MAPPED_FILES
    _SC_MEMLOCK,
#define _SC_MEMLOCK _SC_MEMLOCK
    _SC_MEMLOCK_RANGE,
#define _SC_MEMLOCK_RANGE _SC_MEMLOCK_RANGE
    _SC_MEMORY_PROTECTION,
#define _SC_MEMORY_PROTECTION _SC_MEMORY_PROTECTION
    _SC_MESSAGE_PASSING,
#define _SC_MESSAGE_PASSING _SC_MESSAGE_PASSING
    _SC_MONOTONIC_CLOCK,
#define _SC_MONOTONIC_CLOCK _SC_MONOTONIC_CLOCK
    _SC_MQ_OPEN_MAX,
#define _SC_MQ_OPEN_MAX _SC_MQ_OPEN_MAX
    _SC_MQ_PRIO_MAX,
#define _SC_MQ_PRIO_MAX _SC_MQ_PRIO_MAX
    _SC_NGROUPS_MAX,
#define _SC_NGROUPS_MAX _SC_NGROUPS_MAX
    _SC_OPEN_MAX,
#define _SC_OPEN_MAX _SC_OPEN_MAX
    _SC_PAGE_SIZE,
#define _SC_PAGESIZE  _SC_PAGE_SIZE
#define _SC_PAGE_SIZE _SC_PAGE_SIZE
    _SC_PRIORITIZED_IO,
#define _SC_PRIORITIZED_IO _SC_PRIORITIZED_IO
    _SC_PRIORITY_SCHEDULING,
#define _SC_PRIORITY_SCHEDULING _SC_PRIORITY_SCHEDULING
    _SC_RAW_SOCKETS,
#define _SC_RAW_SOCKETS _SC_RAW_SOCKETS
    _SC_RE_DUP_MAX,
#define _SC_RE_DUP_MAX _SC_RE_DUP_MAX
    _SC_READER_WRITER_LOCKS,
#define _SC_READER_WRITER_LOCKS _SC_READER_WRITER_LOCKS
    _SC_REALTIME_SIGNALS,
#define _SC_REALTIME_SIGNALS _SC_REALTIME_SIGNALS
    _SC_REGEXP,
#define _SC_REGEXP _SC_REGEXP
    _SC_RTSIG_MAX,
#define _SC_RTSIG_MAX _SC_RTSIG_MAX
    _SC_SAVED_IDS,
#define _SC_SAVED_IDS _SC_SAVED_IDS
    _SC_SEM_NSEMS_MAX,
#define _SC_SEM_NSEMS_MAX _SC_SEM_NSEMS_MAX
    _SC_SEM_VALUE_MAX,
#define _SC_SEM_VALUE_MAX _SC_SEM_VALUE_MAX
    _SC_SEMAPHORES,
#define _SC_SEMAPHORES _SC_SEMAPHORES
    _SC_SHARED_MEMORY_OBJECTS,
#define _SC_SHARED_MEMORY_OBJECTS _SC_SHARED_MEMORY_OBJECTS
    _SC_SHELL,
#define _SC_SHELL _SC_SHELL
    _SC_SIGQUEUE_MAX,
#define _SC_SIGQUEUE_MAX _SC_SIGQUEUE_MAX
    _SC_SPAWN,
#define _SC_SPAWN _SC_SPAWN
    _SC_SPIN_LOCKS,
#define _SC_SPIN_LOCKS _SC_SPIN_LOCKS
    _SC_SPORADIC_SERVER,
#define _SC_SPORADIC_SERVER _SC_SPORADIC_SERVER
    _SC_SS_REPL_MAX,
#define _SC_SS_REPL_MAX _SC_SS_REPL_MAX
    _SC_STREAM_MAX,
#define _SC_STREAM_MAX _SC_STREAM_MAX
    _SC_SYMLOOP_MAX,
#define _SC_SYMLOOP_MAX _SC_SYMLOOP_MAX
    _SC_SYNCHRONIZED_IO,
#define _SC_SYNCHRONIZED_IO _SC_SYNCHRONIZED_IO
    _SC_THREAD_ATTR_STACKADDR,
#define _SC_THREAD_ATTR_STACKADDR _SC_THREAD_ATTR_STACKADDR
    _SC_THREAD_ATTR_STACKSIZE,
#define _SC_THREAD_ATTR_STACKSIZE _SC_THREAD_ATTR_STACKSIZE
    _SC_THREAD_CPUTIME,
#define _SC_THREAD_CPUTIME _SC_THREAD_CPUTIME
    _SC_THREAD_DESTRUCTOR_ITERATIONS,
#define _SC_THREAD_DESTRUCTOR_ITERATIONS _SC_THREAD_DESTRUCTOR_ITERATIONS
    _SC_THREAD_KEYS_MAX,
#define _SC_THREAD_KEYS_MAX _SC_THREAD_KEYS_MAX
    _SC_THREAD_PRIO_INHERIT,
#define _SC_THREAD_PRIO_INHERIT _SC_THREAD_PRIO_INHERIT
    _SC_THREAD_PRIO_PROTECT,
#define _SC_THREAD_PRIO_PROTECT _SC_THREAD_PRIO_PROTECT
    _SC_THREAD_PRIORITY_SCHEDULING,
#define _SC_THREAD_PRIORITY_SCHEDULING _SC_THREAD_PRIORITY_SCHEDULING
    _SC_THREAD_PROCESS_SHARED,
#define _SC_THREAD_PROCESS_SHARED _SC_THREAD_PROCESS_SHARED
    _SC_THREAD_ROBUST_PRIO_INHERIT,
#define _SC_THREAD_ROBUST_PRIO_INHERIT _SC_THREAD_ROBUST_PRIO_INHERIT
    _SC_THREAD_ROBUST_PRIO_PROTECT,
#define _SC_THREAD_ROBUST_PRIO_PROTECT _SC_THREAD_ROBUST_PRIO_PROTECT
    _SC_THREAD_SAFE_FUNCTIONS,
#define _SC_THREAD_SAFE_FUNCTIONS _SC_THREAD_SAFE_FUNCTIONS
    _SC_THREAD_SPORADIC_SERVER,
#define _SC_THREAD_SPORADIC_SERVER _SC_THREAD_SPORADIC_SERVER
    _SC_THREAD_STACK_MIN,
#define _SC_THREAD_STACK_MIN _SC_THREAD_STACK_MIN
    _SC_THREAD_THREADS_MAX,
#define _SC_THREAD_THREADS_MAX _SC_THREAD_THREADS_MAX
    _SC_THREADS,
#define _SC_THREADS _SC_THREADS
    _SC_TIMEOUTS,
#define _SC_TIMEOUTS _SC_TIMEOUTS
    _SC_TIMER_MAX,
#define _SC_TIMER_MAX _SC_TIMER_MAX
    _SC_TIMERS,
#define _SC_TIMERS _SC_TIMERS
    _SC_TRACE,
#define _SC_TRACE _SC_TRACE
    _SC_TRACE_EVENT_FILTER,
#define _SC_TRACE_EVENT_FILTER _SC_TRACE_EVENT_FILTER
    _SC_TRACE_EVENT_NAME_MAX,
#define _SC_TRACE_EVENT_NAME_MAX _SC_TRACE_EVENT_NAME_MAX
    _SC_TRACE_INHERIT,
#define _SC_TRACE_INHERIT _SC_TRACE_INHERIT
    _SC_TRACE_LOG,
#define _SC_TRACE_LOG _SC_TRACE_LOG
    _SC_TRACE_NAME_MAX,
#define _SC_TRACE_NAME_MAX _SC_TRACE_NAME_MAX
    _SC_TRACE_SYS_MAX,
#define _SC_TRACE_SYS_MAX _SC_TRACE_SYS_MAX
    _SC_TRACE_USER_EVENT_MAX,
#define _SC_TRACE_USER_EVENT_MAX _SC_TRACE_USER_EVENT_MAX
    _SC_TTY_NAME_MAX,
#define _SC_TTY_NAME_MAX _SC_TTY_NAME_MAX
    _SC_TYPED_MEMORY_OBJECTS,
#define _SC_TYPED_MEMORY_OBJECTS _SC_TYPED_MEMORY_OBJECTS
    _SC_TZNAME_MAX,
#define _SC_TZNAME_MAX _SC_TZNAME_MAX
    _SC_V7_ILP32_OFF32,
#define _SC_V7_ILP32_OFF32 _SC_V7_ILP32_OFF32
    _SC_V7_ILP32_OFFBIG,
#define _SC_V7_ILP32_OFFBIG _SC_V7_ILP32_OFFBIG
    _SC_V7_LP64_OFF64,
#define _SC_V7_LP64_OFF64 _SC_V7_LP64_OFF64
    _SC_V7_LPBIG_OFFBIG,
#define _SC_V7_LPBIG_OFFBIG _SC_V7_LPBIG_OFFBIG
    _SC_V6_ILP32_OFF32,
#define _SC_V6_ILP32_OFF32 _SC_V6_ILP32_OFF32
    _SC_V6_ILP32_OFFBIG,
#define _SC_V6_ILP32_OFFBIG _SC_V6_ILP32_OFFBIG
    _SC_V6_LP64_OFF64,
#define _SC_V6_LP64_OFF64 _SC_V6_LP64_OFF64
    _SC_V6_LPBIG_OFFBIG,
#define _SC_V6_LPBIG_OFFBIG _SC_V6_LPBIG_OFFBIG
    _SC_VERSION,
#define _SC_VERSION _SC_VERSION
    _SC_XOPEN_CRYPT,
#define _SC_XOPEN_CRYPT _SC_XOPEN_CRYPT
    _SC_XOPEN_ENH_I18N,
#define _SC_XOPEN_ENH_I18N _SC_XOPEN_ENH_I18N
    _SC_XOPEN_REALTIME,
#define _SC_XOPEN_REALTIME _SC_XOPEN_REALTIME
    _SC_XOPEN_REALTIME_THREADS,
#define _SC_XOPEN_REALTIME_THREADS _SC_XOPEN_REALTIME_THREADS
    _SC_XOPEN_SHM,
#define _SC_XOPEN_SHM _SC_XOPEN_SHM
    _SC_XOPEN_STREAMS,
#define _SC_XOPEN_STREAMS _SC_XOPEN_STREAMS
    _SC_XOPEN_UNIX,
#define _SC_XOPEN_UNIX _SC_XOPEN_UNIX
    _SC_XOPEN_UUCP,
#define _SC_XOPEN_UUCP _SC_XOPEN_UUCP
    _SC_XOPEN_VERSION
#define _SC_XOPEN_VERSION _SC_XOPEN_VERSION
};

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define _POSIX_VDISABLE (254)

typedef unsigned int useconds_t;

extern char **environ;

void *sbrk(intptr_t increment);
void _exit(int status) __attribute__((__noreturn__));
pid_t fork(void);
pid_t getpid(void);
uid_t getuid(void);
gid_t getgid(void);
uid_t geteuid(void);
gid_t getegid(void);
pid_t getppid(void);
pid_t getsid(pid_t pid);
int setuid(uid_t uid);
int setgid(gid_t gid);
int seteuid(uid_t uid);
int setegid(gid_t gid);
int setpgid(pid_t pid, pid_t pgid);
pid_t setsid(void);
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
int fchdir(int fd);
off_t lseek(int fd, off_t offset, int whence);
unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *pathname, uid_t owner, gid_t group);
int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
int pause(void);
size_t confstr(int name, char *buf, size_t len);
int sysconf(int name);
long pathconf(const char *path, int name);
long fpathconf(int fd, int name);
int gethostname(char *name, size_t len);
int getgroups(int size, gid_t list[]);
int getpagesize(void);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);

int execl(const char *path, const char *arg, ...);
int execle(const char *path, const char *arg, ...);
int execlp(const char *name, const char *arg, ...);
int execv(const char *path, char *const args[]);
int execve(const char *path, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);

int isatty(int fd);
int tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgid);
int ftruncate(int fd, off_t length);
int truncate(const char *path, off_t length);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int pipe(int pipefd[2]);
int unlink(const char *pathname);
int rmdir(const char *pathname);
int access(const char *pathname, int mode);
int faccessat(int dirfd, const char *pathname, int mode, int flags);
ssize_t readlink(const char *__restrict pathname, char *__restrict buf, size_t bufsiz);
int symlink(const char *target, const char *linkpath);
int link(const char *oldpath, const char *newpath);
void sync(void);
int fsync(int fd);

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

int nice(int inc);
unsigned int alarm(unsigned int seconds);
char *ttyname(int fd);
int ttyname_r(int fd, char *buf, size_t buflen);

char *getlogin(void);
int getlogin_r(char *buf, size_t bufsize);

int getopt(int argc, char *const argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UNISTD_H */
