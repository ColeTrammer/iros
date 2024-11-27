#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/sprintf.html
extern "C" auto sprintf(char* __restrict buffer, char const* __restrict format, ...) -> int {
    va_list args;
    va_start(args, format);
    auto result = vsprintf(buffer, format, args);
    va_end(args);
    return result;
}
