#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/printf.html
extern "C" int printf(char const* __restrict format, ...) {
    va_list args;
    va_start(args, format);
    auto result = vprintf(format, args);
    va_end(args);
    return result;
}
