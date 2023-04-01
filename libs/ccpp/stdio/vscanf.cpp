#include <stdarg.h>
#include <stdio.h>

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vscanf.html
extern "C" int vscanf(char const* __restrict format, va_list args) {
    return vfscanf(stdin, format, args);
}
