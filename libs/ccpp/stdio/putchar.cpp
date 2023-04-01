#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putchar.html
extern "C" int putchar(int ch) {
    return putc(ch, stdout);
}
