#include <ccpp/bits/file_implementation.h>
#include <errno.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fileno_unlocked.html
extern "C" int fileno_unlocked(FILE* file) {
    auto result = file->get_unlocked().file.file_descriptor();
    if (result == -1) {
        errno = EBADF;
        return -1;
    }
    return result;
}
