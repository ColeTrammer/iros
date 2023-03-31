#include <ccpp/bits/file_implementation.h>
#include <unistd.h>

namespace ccpp {
extern "C" long ftell(FILE* file) {
    return file->locked.with_lock([](File& file) {
        auto result = lseek(file.file.file_descriptor(), 0, SEEK_CUR);
        if (result == -1) {
            return -1L;
        }
        if (!di::math::representable_as<long>(result)) {
            errno = ERANGE;
            return -1L;
        }
        return long(result);
    });
}
}
