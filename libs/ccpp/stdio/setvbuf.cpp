#include <ccpp/bits/file_implementation.h>
#include <stdlib.h>
#include <string.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/setvbuf.html
extern "C" int setvbuf(FILE* __restrict file, char* __restrict buffer, int mode, size_t size) {
    if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
        errno = EINVAL;
        return -1;
    }

    auto ownership = BufferOwnership::UserProvided;
    if (!buffer && mode != _IONBF) {
        if (buffer = static_cast<char*>(malloc(size)); !buffer) {
            return -1;
        }
        ownership = BufferOwnership::Owned;
    }

    auto guard = di::ScopedLock(file->locked.get_lock());
    auto& inner = file->get_unlocked();
    if (inner.buffer_ownership == BufferOwnership::Owned) {
        free(inner.buffer);
    }

    inner.buffer = reinterpret_cast<byte*>(buffer);
    inner.buffer_capacity = size;
    inner.buffer_mode = static_cast<BufferMode>(mode);
    inner.buffer_ownership = ownership;
    return 0;
}
}
