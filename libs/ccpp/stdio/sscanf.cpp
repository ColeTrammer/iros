#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/sscanf.html
extern "C" auto sscanf(char const* buffer, char const* __restrict format, ...) -> int {
    va_list args;
    va_start(args, format);
    auto result = vsscanf(buffer, format, args);
    va_end(args);
    return result;
}
