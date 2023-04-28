#pragma once

#include <ccpp/bits/stat.h>

__CCPP_BEGIN_DECLARATIONS
// NOTE: These constants are chosen to align with the values used by the Linux kernel.
#define S_IFMT   00170000
#define S_IFBLK  0060000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000

#define S_ISBLK(mode)  (((mode) &S_IFMT) == S_IFBLK)
#define S_ISCHR(mode)  (((mode) &S_IFMT) == S_IFCHR)
#define S_ISFIFO(mode) (((mode) &S_IFMT) == S_IFIFO)
#define S_ISREG(mode)  (((mode) &S_IFMT) == S_IFREG)
#define S_ISDIR(mode)  (((mode) &S_IFMT) == S_IFDIR)
#define S_ISLNK(mode)  (((mode) &S_IFMT) == S_IFLNK)
#define S_ISSOCK(mode) (((mode) &S_IFMT) == S_IFSOCK)

#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100

#define S_IRWXG 070
#define S_IRGRP 040
#define S_IWGRP 020
#define S_IXGRP 010

#define S_IRWXO 07
#define S_IROTH 04
#define S_IWOTH 02
#define S_IXOTH 01

#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

int stat(char const* __CCPP_RESTRICT __path, struct stat* __CCPP_RESTRICT __info);
int lstat(char const* __CCPP_RESTRICT __path, struct stat* __CCPP_RESTRICT __info);
int fstat(int __fd, struct stat* __info);

int chmod(char const* __path, mode_t mode);
int fchmod(int __fd, mode_t mode);

int mkdir(char const* __path, mode_t mode);

__CCPP_END_DECLARATIONS
