#include <ccpp/bits/file_implementation.h>
#include <ccpp/bits/printf_implementation.h>
#include <stdarg.h>
#include <stdio.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vfprintf.html
extern "C" int vfprintf(FILE* __restrict file, char const* __restrict format, va_list args) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return STDIO_TRY(printf_implementation(
        [&](di::TransparentStringView bytes) -> di::Expected<void, dius::PosixCode> {
            // Make sure to write the entire buffer.
            while (!bytes.empty()) {
                auto result = fwrite_unlocked(bytes.data(), 1, bytes.size(), file);
                if (result == 0) {
                    return di::Unexpected(dius::PosixError(errno));
                }
                bytes = bytes | di::drop(result);
            }
            return {};
        },
        format, args));
}
}
