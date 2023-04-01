#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putc.html
extern "C" int putc(int ch, FILE* file) {
    return fputc(ch, file);
}
