#include <ccpp/bits/file_implementation.h>

// NOTE: this is an extension of fputc(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fputc.html
extern "C" int fputc_unlocked(int ch, FILE* file) {
    (void) ch;
    (void) file;
    return EOF;
}
