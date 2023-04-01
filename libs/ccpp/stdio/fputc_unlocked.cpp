#include <ccpp/bits/file_implementation.h>

namespace ccpp {
// NOTE: this is an extension of fputc(), which does not lock file.
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/fputc.html
extern "C" int fputc_unlocked(int ch, FILE* file) {
    auto& inner = file->get_unlocked();
    if (inner.has_error()) {
        return EOF;
    }

    // Ensure the stream is writable.
    STDIO_TRY(inner.mark_as_writable());

    // Write char directly if not using buffering.
    auto byte = di::byte(ch);
    if (inner.buffer_mode == BufferMode::NotBuffered) {
        STDIO_TRY_OR_MARK_ERROR(inner, inner.file.write_exactly({ &byte, 1 }));
        return ch;
    }

    // Check if the internal buffer is full.
    if (inner.buffer_size == inner.buffer_capacity) {
        if (fflush_unlocked(file)) {
            return EOF;
        }
    }

    // Now append the new character.
    inner.buffer[inner.buffer_size++] = byte;

    // Flush if the new character is a newline and in line-buffered mode.
    if (inner.buffer_mode == BufferMode::LineBuffered && ch == '\n') {
        if (fflush_unlocked(file)) {
            return EOF;
        }
    }

    return ch;
}
}
