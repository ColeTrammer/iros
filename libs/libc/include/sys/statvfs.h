#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H 1

#include <bits/fsblkcnt_t.h>
#include <bits/fsfilcnt_t.h>

#define ST_RDONLY 1
#define ST_NOSUID 2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct statvfs {
    unsigned long f_bsize;
    unsigned long f_frsize;
    fsblkcnt_t f_blocks;
    fsblkcnt_t f_bfree;
    fsblkcnt_t f_bavail;
    fsfilcnt_t f_files;
    fsfilcnt_t f_ffree;
    fsfilcnt_t f_favail;
    unsigned long f_fsid;
    unsigned long f_flag;
    unsigned long f_namemax;
};

int fstatvfs(int fd, struct statvfs *stat_buf);
int statvfs(const char *__restrict path, struct statvfs *__restrict stat_buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_STATVFS_H */