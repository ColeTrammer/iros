#include <ccpp/bits/file_implementation.h>
#include <unistd.h>

// NOTE: this is an extension of fflush(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fflush.html
extern "C" int fflush_unlocked(FILE* file) {
    auto& inner = file->get_unlocked();

    if (inner.buffer_size == 0) {
        return 0;
    }

    if (inner.readable()) {
        // Discard the buffered data and rewind the stream.
        // If seeking fails, the underlying stream is not-seekable, and so just ignore the error.
        int save_errno = errno;
        off_t result = lseek(inner.file.file_descriptor(), -off_t(inner.buffer_size), SEEK_CUR);
        errno = save_errno;

        if (result == -1) {
            return 0;
        }
        inner.buffer_size = 0;
        inner.buffer_offset = 0;
    } else {
        // Flush any pending data by writing to disk.
        inner.buffer_offset = 0;
        auto buffer_size = di::exchange(inner.buffer_size, 0);
        STDIO_TRY_OR_MARK_ERROR(inner, inner.file.write_exactly({ inner.buffer, buffer_size }));
    }

    return 0;
}
