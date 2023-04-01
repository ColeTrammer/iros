#include <ccpp/bits/file_implementation.h>
#include <ccpp/bits/scanf_implementation.h>
#include <stdarg.h>
#include <stdio.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vsscanf.html
extern "C" int vsscanf(char const* __restrict buffer, char const* __restrict format, va_list args) {
    return STDIO_TRY(scanf_implementation(
        [&]() -> di::Expected<di::Optional<char>, dius::PosixCode> {
            if (*buffer == '\0') {
                return di::nullopt;
            }
            return *buffer++;
        },
        format, args));
}
}
