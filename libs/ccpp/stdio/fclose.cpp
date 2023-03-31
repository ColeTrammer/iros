#include <ccpp/bits/file_implementation.h>

namespace ccpp {
extern "C" int fclose(FILE* file) {
    if (fflush(file)) {
        return EOF;
    }

    // No need to lock here, as it is undefined behavior to try to use a FILE after it has been closed.
    STDIO_TRY(file->get_unlocked().file.close());

    auto to_drop = FileHandle(file);

    return 0;
}
}
