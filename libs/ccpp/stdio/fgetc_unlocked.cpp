#include <ccpp/bits/file_implementation.h>

// NOTE: this is an extension of fgetc(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fgetc.html
extern "C" int fgetc_unlocked(FILE* file) {
    (void) file;
    return EOF;
}
