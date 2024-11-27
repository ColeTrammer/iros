#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fgetc.html
extern "C" auto fgetc(FILE* file) -> int {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fgetc_unlocked(file);
}
