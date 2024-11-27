#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putc_unlocked.html
extern "C" auto putc_unlocked(int ch, FILE* file) -> int {
    return fputc_unlocked(ch, file);
}
