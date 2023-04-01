#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putc_unlocked.html
extern "C" int putc_unlocked(int ch, FILE* file) {
    return fputc_unlocked(ch, file);
}
