#include <ccpp/bits/file_implementation.h>
#include <unistd.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fseek.html
extern "C" int fseek(FILE* file, long offset, int origin) {
    return file->locked.with_lock([&](File& inner) {
        if (fflush_unlocked(file)) {
            return -1;
        }

        auto result = lseek(inner.file.file_descriptor(), offset, origin);
        if (result == -1) {
            return -1;
        }
        return 0;
    });
}
}
