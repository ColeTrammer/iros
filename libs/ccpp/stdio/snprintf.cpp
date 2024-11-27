#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/snprintf.html
extern "C" auto snprintf(char* __restrict buffer, size_t size, char const* __restrict format, ...) -> int {
    va_list args;
    va_start(args, format);
    auto result = vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}
