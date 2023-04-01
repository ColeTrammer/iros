#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fwrite.html
extern "C" size_t fwrite(void const* __restrict buffer, size_t size, size_t count, FILE* __restrict file) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fwrite_unlocked(buffer, size, count, file);
}
