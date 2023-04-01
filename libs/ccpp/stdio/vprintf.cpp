#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vprintf.html
extern "C" int vprintf(char const* __restrict format, va_list args) {
    return vfprintf(stdout, format, args);
}
