#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fprintf.html
extern "C" int fprintf(FILE* __restrict file, char const* __restrict format, ...) {
    va_list args;
    va_start(args, format);
    auto result = vfprintf(file, format, args);
    va_end(args);
    return result;
}
