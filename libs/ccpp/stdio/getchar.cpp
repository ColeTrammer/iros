#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getchar.html
extern "C" int getchar(void) {
    return getc(stdin);
}
