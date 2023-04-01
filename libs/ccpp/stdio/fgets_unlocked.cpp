#include <ccpp/bits/file_implementation.h>

// NOTE: this is an extension of fgets(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fgets.html
extern "C" char* fgets_unlocked(char* __restrict str, int count, FILE* __restrict file) {
    if (count < 1) {
        errno = EINVAL;
        return nullptr;
    }
    if (count == 1) {
        *str = '\0';
        return 0;
    }

    auto* current = str;
    for (int nbytes = 0; nbytes < count - 1; nbytes++) {
        auto ch = fgetc_unlocked(file);
        if (ch == EOF) {
            return nullptr;
        }
        *current++ = char(ch);
    }
    *current = '\0';
    return str;
}
