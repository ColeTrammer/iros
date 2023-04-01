#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/getchar_unlocked.html
extern "C" int getchar_unlocked(void) {
    return getc_unlocked(stdin);
}
