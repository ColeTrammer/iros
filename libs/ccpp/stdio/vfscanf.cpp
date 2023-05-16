#include <ccpp/bits/file_implementation.h>
#include <ccpp/bits/scanf_implementation.h>
#include <stdarg.h>
#include <stdio.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/vfscanf.html
extern "C" int vfscanf(FILE* __restrict file, char const* __restrict format, va_list args) {
    auto guard = di::ScopedLock(file->locked.get_lock());
    return STDIO_TRY(scanf_implementation(
        [&]() -> di::Expected<di::Optional<char>, di::GenericCode> {
            auto ch = fgetc_unlocked(file);
            if (ch == EOF) {
                if (file->get_unlocked().has_error()) {
                    return di::Unexpected(di::BasicError(errno));
                }
                return di::nullopt;
            }
            return char(ch);
        },
        format, args));
}
}
