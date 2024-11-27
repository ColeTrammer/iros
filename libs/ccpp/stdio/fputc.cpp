#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fputc.html
extern "C" auto fputc(int ch, FILE* file) -> int {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return fputc_unlocked(ch, file);
}
