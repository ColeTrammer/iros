#ifndef _ERRNO_H
#define _ERRNO_H 1

#define ESTARTERRNO 0
#define EIO         1
#define ENOMEM      2
#define EINVAL      3
#define ENOENT      4
#define ENOEXEC     5
#define EISDIR      6
#define EMAXERRNO   7

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

extern int errno;

#define errno errno

#endif /* __is_kernel */

#ifdef __libc_internal
void init_errno();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ERRNO_H */