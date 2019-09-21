#ifndef _SYS_STAT_H
#define _SYS_STAT_H 1

#include <sys/types.h>
#include <time.h>

#define S_IFMT (~07777U)
#define S_IFBLK 010000
#define S_IFCHR 020000
#define S_IFIFO 030000
#define S_IFREG 040000
#define S_IFDIR 050000
#define S_IFLNK 060000
#define S_IFSOCK 070000

#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISOCK(m) (((m) & S_IFMT) == S_IFSOCK)

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
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

int stat(const char *restrict path, struct stat *restrict stat_struct);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_STAT_H */