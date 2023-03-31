#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

// NOTE: this must be kept in sync with <iris/uapi/error.h>.
#define EAFNOSUPPORT    1
#define EADDRINUSE      2
#define EADDRNOTAVAIL   3
#define EISCONN         4
#define E2BIG           5
#define EDOM            6
#define EFAULT          7
#define EBADF           8
#define EBADMSG         9
#define EPIPE           10
#define ECONNABORTED    11
#define EALREADY        12
#define ECONNREFUSED    13
#define ECONNRESET      14
#define EXDEV           15
#define EDESTADDRREQ    16
#define EBUSY           17
#define ENOTEMPTY       18
#define ENOEXEC         19
#define EEXIST          20
#define EFBIG           21
#define ENAMETOOLONG    22
#define ENOSYS          23
#define EHOSTUNREACH    24
#define EIDRM           25
#define EILSEQ          26
#define ENOTTY          27
#define EINTR           28
#define EINVAL          29
#define ESPIPE          30
#define EIO             31
#define EISDIR          32
#define EMSGSIZE        33
#define ENETDOWN        34
#define ENETRESET       35
#define ENETUNREACH     36
#define ENOBUFS         37
#define ECHILD          38
#define ENOLINK         39
#define ENOLCK          40
#define ENODATA         41
#define ENOMSG          42
#define ENOPROTOOPT     43
#define ENOSPC          44
#define ENOSR           45
#define ENXIO           46
#define ENODEV          47
#define ENOENT          48
#define ESRCH           49
#define ENOTDIR         50
#define ENOTSOCK        51
#define ENOSTR          52
#define ENOTCONN        53
#define ENOMEM          54
#define ENOTSUP         55
#define ECANCELED       56
#define EINPROGRESS     57
#define EPERM           58
#define EOPNOTSUPP      59
#define EWOULDBLOCK     60
#define EOWNERDEAD      61
#define EACCES          62
#define EPROTO          63
#define EPROTONOSUPPORT 64
#define EROFS           65
#define EDEADLK         66
#define EAGAIN          EWOULDBLOCK
#define ERANGE          67
#define ENOTRECOVERABLE 68
#define ETIME           69
#define ETXTBSY         70
#define ETIMEDOUT       71
#define ENFILE          72
#define EMFILE          73
#define EMLINK          74
#define ELOOP           75
#define EOVERFLOW       76
#define EPROTOTYPE      77

__CCPP_END_DECLARATIONS
