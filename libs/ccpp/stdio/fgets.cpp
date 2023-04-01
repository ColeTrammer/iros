#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fgets.html
extern "C" char* fgets(char* __restrict str, int count, FILE* __restrict file) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fgets_unlocked(str, count, file);
}
