#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/scanf.html
extern "C" int scanf(char const* __restrict format, ...) {
    va_list args;
    va_start(args, format);
    auto result = vscanf(format, args);
    va_end(args);
    return result;
}
