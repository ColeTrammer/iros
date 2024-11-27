#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putchar.html
extern "C" auto putchar(int ch) -> int {
    return putc(ch, stdout);
}
