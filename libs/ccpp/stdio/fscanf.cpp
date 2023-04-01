#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fscanf.html
extern "C" int fscanf(FILE* __restrict file, char const* __restrict format, ...) {
    va_list args;
    va_start(args, format);
    auto result = vfscanf(file, format, args);
    va_end(args);
    return result;
}
