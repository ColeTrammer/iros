#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putchar_unlocked.html
extern "C" int putchar_unlocked(int ch) {
    return fputc_unlocked(ch, stdout);
}
