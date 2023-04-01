#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fflush.html
extern "C" int fflush(FILE* file) {
    if (!file) {
        // FIXME: This should flush all streams.
        return 0;
    }

    auto guard = di::ScopedLock(file->locked.get_lock());
    return fflush_unlocked(file);
}
