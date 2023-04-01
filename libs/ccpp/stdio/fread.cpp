#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fread.html
extern "C" size_t fread(void* __restrict buffer, size_t size, size_t count, FILE* __restrict file) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fread_unlocked(buffer, size, count, file);
}
