#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fileno.html
extern "C" auto fileno(FILE* file) -> int {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fileno_unlocked(file);
}
