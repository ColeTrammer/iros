#ifndef _ERRNO_H
#define _ERRNO_H 1

#define ESTARTERRNO     0
#define EIO             1
#define ENOMEM          2
#define EINVAL          3
#define ENOENT          4
#define ENOEXEC         5
#define EISDIR          6
#define ERANGE          7
#define ENOSPC          8
#define ENOTTY          9
#define EEXIST          10
#define EBADFD          11
#define ENOTEMPTY       12
#define ENOTDIR         13
#define EPERM           14
#define EAFNOSUPPORT    15
#define EADDRINUSE      16
#define EADDRNOTAVAIL   17
#define EISCONN         18
#define E2BIG           19
#define EDOM            20
#define EFAULT          21
#define EBADF           22
#define EPIPE           23
#define ECONNABORTED    24
#define EALREADY        25
#define ECONNREFUSED    26
#define ECONNRESET      27
#define EXDEV           28
#define EDESTADDRREQ    29
#define EBUSY           30
#define EFBIG           31
#define ENAMETOOLONG    32
#define ENOSYS          33
#define EHOSTUNREACH    34
#define EINTR           35
#define ESPIPE          36
#define EMSGSIZE        37
#define ENETDOWN        38
#define ENETRESET       39
#define ENETUNREACH     40
#define ENOBUFS         41
#define ECHILD          42
#define ENOLCK          43
#define ENOMSG          44
#define ENOPROTOOPT     45
#define ENXIO           46
#define ENODEV          47
#define ESRCH           48
#define ENOTSOCK        49
#define ENOTCONN        50
#define EINPROGRESS     51
#define EOPNOTSUPP      52
#define EWOULDBLOCK     53
#define EAGAIN          EWOULDBLOCK
#define EACCES          54
#define EPROTONOSUPPORT 55
#define EROFS           56
#define EDEADLK         57
#define ETIMEDOUT       58
#define ENFILE          59
#define EMFILE          60
#define EMLINK          61
#define ELOOP           62
#define EPROTOTYPE      63
#define EILSEQ          64
#define ENOTSUP         65
#define EMAXERRNO       66

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(__is_kernel) && !defined(__is_libk)

#define __SYSCALL_TO_ERRNO(val) \
    do {                        \
        if (val < 0) {          \
            errno = -(val);     \
            return -1;          \
        }                       \
        errno = 0;              \
        return (val);           \
    } while (0)

extern __thread int errno;
#define errno errno

#else

extern int errno;
#define errno errno

#endif /* !defined(__is_kernel) && !defined(__is_libk) */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ERRNO_H */