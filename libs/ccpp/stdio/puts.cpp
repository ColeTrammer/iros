#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/puts.html
extern "C" auto puts(char const* str) -> int {
    auto guard = di::ScopedLock(stdout->locked.get_lock());
    if (fputs_unlocked(str, stdout)) {
        return EOF;
    }
    return fputc_unlocked('\n', stdout);
}
