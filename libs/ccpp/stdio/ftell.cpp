#include <ccpp/bits/file_implementation.h>
#include <unistd.h>

namespace ccpp {
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/ftell.html
extern "C" long ftell(FILE* file) {
    return file->locked.with_lock([](File& file) {
        auto result = lseek(file.file.file_descriptor(), 0, SEEK_CUR);
        if (result == -1) {
            return -1L;
        }

        // Adjust if there is any buffered data.
        if (file.buffer_size > 0) {
            if (file.readable()) {
                result -= off_t(file.buffer_size);
            } else {
                result += off_t(file.buffer_size);
            }
        }

        if (!di::math::representable_as<long>(result)) {
            errno = EOVERFLOW;
            return -1L;
        }
        return long(result);
    });
}
}
