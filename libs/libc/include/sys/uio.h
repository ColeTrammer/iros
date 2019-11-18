#ifndef _SYS_UIO_H
#define _SYS_UIO_H 1

#include <bits/size_t.h>
#include <bits/ssize_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct iovec {
    void *iov_base;
    size_t iov_len;
};

ssize_t readv(int fd, const struct iovec *vec, int num);
ssize_t writev(int fd, const struct iovec *vec, int num);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_UIO_H */