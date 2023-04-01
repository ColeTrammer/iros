#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fclose.html
extern "C" int fclose(FILE* file) {
    // No need to lock here, as it is undefined behavior to try to use a FILE after it has been closed.
    bool error = false;
    if (fflush_unlocked(file)) {
        error = true;
    }

    auto result = file->get_unlocked().file.close();
    if (!result) {
        errno = int(result.error().value());
        error = true;
    }

    delete file;

    return error ? EOF : 0;
}
