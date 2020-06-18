#ifndef _SYS_STAT_H
#define _SYS_STAT_H 1

#include <bits/blkcnt_t.h>
#include <bits/blksize_t.h>
#include <bits/dev_t.h>
#include <bits/gid_t.h>
#include <bits/ino_t.h>
#include <bits/mode_t.h>
#include <bits/nlink_t.h>
#include <bits/off_t.h>
#include <bits/uid_t.h>
#include <time.h>

#define S_IFMT   (~0xFFFU)
#define S_IFBLK  0x6000
#define S_IFCHR  0x2000
#define S_IFIFO  0x1000
#define S_IFREG  0x8000
#define S_IFDIR  0x4000
#define S_IFLNK  0xA000
#define S_IFSOCK 0xC000

#define S_ISBLK(m)  (((m) &S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m) &S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) &S_IFMT) == S_IFIFO)
#define S_ISREG(m)  (((m) &S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) &S_IFMT) == S_IFDIR)
#define S_ISLNK(m)  (((m) &S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) &S_IFMT) == S_IFSOCK)

#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

#define S_ISUID 04000
#define S_ISGID 02000
#define S_SIVTX 01000

#define UTIME_OMIT -1
#define UTIME_NOW  -2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

int stat(const char *__restrict path, struct stat *__restrict stat_struct);
int fstat(int fd, struct stat *stat_struct);
int fstatat(int fd, const char *__restrict path, struct stat *__restrict stat_struct, int flag);
int lstat(const char *__restrict path, struct stat *__restrict stat_struct);
int mkdir(const char *path, mode_t mode);
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
mode_t umask(mode_t mask);
int mkfifo(const char *pathname, mode_t mode);
int mknod(const char *pathname, mode_t mode, dev_t dev);
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2], int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_STAT_H */
