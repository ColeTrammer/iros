#pragma once

#include <ext/error.h>
#include <fcntl.h>
#include <liim/enum.h>
#include <liim/forward.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace Ext::System {
template<typename T>
using Result = LIIM::Result<T, SystemError>;

Result<Path> realpath(PathView path);

enum class OpenMode : int {
    Create = O_CREAT,
    Exclusive = O_EXCL,
    NoControllingTerminal = O_NOCTTY,
    Truncate = O_TRUNC,
    Append = O_APPEND,
    SyncWrites = O_DSYNC,
    NonBlocking = O_NONBLOCK,
    SyncReads = O_RSYNC,
    Sync = O_SYNC,
    AccessMode = O_ACCMODE,
    ReadOnly = O_RDONLY,
    ReadWrite = O_RDWR,
    WriteOnly = O_WRONLY,
};

LIIM_DEFINE_BITWISE_OPERATIONS(OpenMode)

Result<int> open(PathView path, OpenMode open_mode, mode_t create_mode = 0666);

enum class Protection : int {
    None = PROT_NONE,
    Executable = PROT_EXEC,
    Readable = PROT_READ,
    Writeable = PROT_WRITE,
};

LIIM_DEFINE_BITWISE_OPERATIONS(Protection)

enum class MapFlags {
    Shared = MAP_SHARED,
    Private = MAP_PRIVATE,
    Fixed = MAP_FIXED,
    Anonymous = MAP_ANON,
    Stack = MAP_STACK,
};

LIIM_DEFINE_BITWISE_OPERATIONS(MapFlags)

Result<void*> mmap(void* addr, size_t length, Protection protection, MapFlags flags, Option<int> fd, off_t offset);

Result<struct stat> fstat(int fd);

Result<void> chdir(PathView path);

Result<bool> exists(PathView path);
}
