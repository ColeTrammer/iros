#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/scanf.html
extern "C" auto scanf(char const* __restrict format, ...) -> int {
    va_list args;
    va_start(args, format);
    auto result = vscanf(format, args);
    va_end(args);
    return result;
}
