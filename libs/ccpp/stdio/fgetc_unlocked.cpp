#include <ccpp/bits/file_implementation.h>

namespace ccpp {
// NOTE: this is an extension of fgetc(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fgetc.html
extern "C" int fgetc_unlocked(FILE* file) {
    auto& inner = file->get_unlocked();

    // If we're are EOF or already error'ed, ignore.
    if (inner.at_eof() || inner.has_error()) {
        return EOF;
    }

    // Ensure the stream is readable.
    STDIO_TRY(inner.mark_as_readable());

    // Read directly if not using buffering.
    if (inner.buffer_mode != BufferMode::FullBuffered) {
        auto byte = 0_b;
        auto nread = STDIO_TRY_OR_MARK_ERROR(inner, inner.file.read_some({ &byte, 1 }));
        if (nread == 0) {
            inner.mark_as_eof();
            return EOF;
        }

        return di::to_integer<int>(byte);
    }

    // If the buffer is empty, try to refill.
    if (inner.buffer_size == 0) {
        auto nread = STDIO_TRY_OR_MARK_ERROR(inner, inner.file.read_some({ inner.buffer, inner.buffer_capacity }));
        if (nread == 0) {
            inner.mark_as_eof();
            return EOF;
        }

        inner.buffer_offset = 0;
        inner.buffer_size = nread;
    }

    // Extract from buffer.
    auto result = di::to_integer<int>(inner.buffer[inner.buffer_offset++]);
    inner.buffer_size -= 1;
    return result;
}
}
