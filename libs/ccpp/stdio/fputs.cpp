#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fputs.html
extern "C" int fputs(char const* __restrict str, FILE* __restrict file) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fputs_unlocked(str, file);
}
