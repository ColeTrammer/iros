#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int pid_t;
typedef unsigned long clock_t;
typedef unsigned int mode_t;
typedef unsigned long dev_t;
typedef long off_t;
typedef unsigned long long ino_t;
typedef unsigned short uid_t;
typedef unsigned short gid_t;
typedef unsigned short nlink_t;
typedef long blksize_t;
typedef long blkcnt_t;
typedef long time_t;

#define __socklen_t_defined
#define __socklen_t uint32_t
typedef __socklen_t socklen_t;

typedef int pthread_attr_t;
typedef int pthread_t;

typedef long ssize_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_TYPES_H */