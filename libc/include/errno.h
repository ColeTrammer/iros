#ifndef _ERRNO_H
#define _ERRNO_H 1

#define EIO    1
#define ENOMEM 2

#define __SYSCALL_TO_ERRNO(val) \
    do {                        \
        if (val < 0) {          \
            errno = -(val);     \
            return -1;          \
        }                       \
        errno = 0;              \
        return (val);           \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int errno;

#define errno errno

#ifdef __libc_internal
void init_errno();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ERRNO_H */