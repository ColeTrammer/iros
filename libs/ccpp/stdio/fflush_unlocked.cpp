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
    } else {
        // Flush any pending data by writing to disk.
        auto result = inner.file.write_exactly({ inner.buffer, inner.buffer_size });
        inner.buffer_size = 0;
        if (!result) {
            inner.mark_as_error();
            errno = int(result.error().value());
            return EOF;
        }
    }

    return 0;
}
