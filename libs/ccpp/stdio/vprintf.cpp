#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vprintf.html
extern "C" auto vprintf(char const* __restrict format, va_list args) -> int {
    return vfprintf(stdout, format, args);
}
