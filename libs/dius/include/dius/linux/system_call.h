#pragma once

#include <asm/unistd.h>
#include <linux/fcntl.h>
#include <linux/io_uring.h>
#include <linux/mman.h>

namespace dius::system {
enum class Number : int {
    io_uring_enter = __NR_io_uring_enter,
    io_uring_setup = __NR_io_uring_setup,
    io_uring_register = __NR_io_uring_register,
    pread = __NR_pread64,
    pwrite = __NR_pwrite64,
    read = __NR_read,
    write = __NR_write,
    close = __NR_close,
    openat = __NR_openat,
    mmap = __NR_mmap,
    munmap = __NR_munmap,
    getdents64 = __NR_getdents64,
    fstatat64 = __NR_newfstatat,
    ftruncate = __NR_ftruncate,
};

using SystemCallArg = unsigned long;
using SystemCallResult = long;
}