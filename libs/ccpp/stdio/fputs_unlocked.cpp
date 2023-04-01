#include <ccpp/bits/file_implementation.h>

// NOTE: this is an extension of fputs(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fputs.html
extern "C" int fputs_unlocked(char const* __restrict str, FILE* __restrict file) {
    int result = 1;
    for (auto ch : di::ZCString(str)) {
        result = fputc_unlocked(ch, file);
        if (result == EOF) {
            return EOF;
        }
    }
    return result;
}
