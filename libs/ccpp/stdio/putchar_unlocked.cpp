#include <ccpp/bits/file_implementation.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/putchar_unlocked.html
extern "C" auto putchar_unlocked(int ch) -> int {
    return fputc_unlocked(ch, stdout);
}
