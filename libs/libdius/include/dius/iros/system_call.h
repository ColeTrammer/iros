#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>

namespace dius::system {
enum class Number : int {
    close = SYS_close,
    openat = SYS_openat,
    read = SYS_read,
    write = SYS_write,
    pread = SYS_pread,
    pwrite = SYS_pwrite,
    mmap = SYS_mmap,
    munmap = SYS_munmap,
};

using SystemCallArg = unsigned long;
using SystemCallResult = long;
}