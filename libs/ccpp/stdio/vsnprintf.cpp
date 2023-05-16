#include <ccpp/bits/file_implementation.h>
#include <ccpp/bits/printf_implementation.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/view/prelude.h>
#include <stdarg.h>
#include <stdio.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vsnprintf.html
extern "C" int vsnprintf(char* __restrict buffer, size_t size, char const* __restrict format, va_list args) {
    if (size == 1) {
        *buffer = '\0';
        size = 0;
    }

    return STDIO_TRY(printf_implementation(
        [&](di::TransparentStringView bytes) -> di::Expected<void, di::GenericCode> {
            if (size <= 1) {
                return {};
            }

            auto truncated = bytes | di::take(size - 1);
            buffer = di::copy(truncated, buffer).out;
            size -= truncated.size();
            *buffer = '\0';
            return {};
        },
        format, args));
}
}
