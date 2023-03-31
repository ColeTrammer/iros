#include <ccpp/bits/file_implementation.h>
#include <unistd.h>

namespace ccpp {
extern "C" int fseek(FILE* file, long offset, int origin) {
    return file->locked.with_lock([&](File& file) {
        auto result = lseek(file.file.file_descriptor(), offset, origin);
        if (result == -1) {
            return 0;
        }
        return 1;
    });
}
}
