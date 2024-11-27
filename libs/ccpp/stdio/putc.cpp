#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putc.html
extern "C" auto putc(int ch, FILE* file) -> int {
    return fputc(ch, file);
}
