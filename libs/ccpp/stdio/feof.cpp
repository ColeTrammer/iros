#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/feof.html
extern "C" int feof(FILE* file) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return feof_unlocked(file);
}
