#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getchar.html
extern "C" auto getchar(void) -> int {
    return getc(stdin);
}
