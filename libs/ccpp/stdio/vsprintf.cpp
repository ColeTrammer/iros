#include <ccpp/bits/file_implementation.h>
#include <ccpp/bits/printf_implementation.h>
#include <di/container/algorithm/prelude.h>
#include <stdarg.h>
#include <stdio.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vsprintf.html
extern "C" int vsprintf(char* __restrict buffer, char const* __restrict format, va_list args) {
    return STDIO_TRY(printf_implementation(
        [&](di::TransparentStringView bytes) -> di::Expected<void, di::GenericCode> {
            buffer = di::copy(bytes, buffer).out;
            *buffer = '\0';
            return {};
        },
        format, args));
}
}
