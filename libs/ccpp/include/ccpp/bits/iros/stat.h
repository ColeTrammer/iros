#pragma once

#include <ccpp/bits/blkcnt_t.h>
#include <ccpp/bits/blksize_t.h>
#include <ccpp/bits/dev_t.h>
#include <ccpp/bits/gid_t.h>
#include <ccpp/bits/ino_t.h>
#include <ccpp/bits/mode_t.h>
#include <ccpp/bits/nlink_t.h>
#include <ccpp/bits/off_t.h>
#include <ccpp/bits/timespec.h>
#include <ccpp/bits/uid_t.h>

__CCPP_BEGIN_DECLARATIONS

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    nlink_t st_nlink;
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    unsigned char __padding1[4];
    dev_t st_rdev;
    off_t st_size;
    blksize_t st_blksize;
    blkcnt_t st_blkcnt;
    struct timespec st_atim;
#define st_atime st_atim.tv_sec
    struct timespec st_mtim;
#define st_mtime st_mtim.tv_sec
    struct timespec st_ctim;
#define st_ctime st_ctim.tv_sec
    unsigned char __padding2[24];
};

__CCPP_END_DECLARATIONS
