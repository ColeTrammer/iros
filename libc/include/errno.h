#ifndef _ERRNO_H
#define _ERRNO_H 1

#define ESTARTERRNO 0
#define EIO         1
#define ENOMEM      2
#define EINVAL      3
#define ENOENT      4
#define ENOEXEC     5
#define EISDIR      6
#define ERANGE      7
#define ENOSPC      8
#define ENOTTY      9
#define EEXIST      10
#define EBADFD      11
#define ENOTEMPTY   12
#define ENOTDIR     13
#define EMAXERRNO   14

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __is_kernel

#define __SYSCALL_TO_ERRNO(val) \
    do {                        \
        if (val < 0) {          \
            errno = -(val);     \
            return -1;          \
        }                       \
        errno = 0;              \
        return (val);           \
    } while (0)

#endif /* __is_kernel */

extern int errno;

#define errno errno

#ifdef __libc_internal
void init_errno();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ERRNO_H */