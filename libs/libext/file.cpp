#include <errno.h>
#include <ext/file.h>
#include <liim/function.h>

namespace Ext {
UniquePtr<File> File::create(const String& path, const String& type) {
    auto* c_file = fopen(path.string(), type.string());
    if (!c_file) {
        return nullptr;
    }
    return make_unique<File>(c_file);
}

File::File(FILE* file) : m_file(file) {}

File::~File() {
    if (m_should_close_file) {
        close();
    }
}

Maybe<ByteBuffer> File::read_all() {
    return {};
}

bool File::read_all_lines(Function<bool(String)> callback, StripTrailingNewlines strip_newlines) {
    char* line = nullptr;
    size_t line_max = 0;
    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, m_file)) > 0) {
        if (strip_newlines == StripTrailingNewlines::Yes && line[line_length - 1] == '\n') {
            line[line_length--] = '\0';
        }

        auto line_string = String(line, static_cast<size_t>(line_length));
        if (!callback(move(line_string))) {
            break;
        }
    }
    free(line);

    if (ferror(m_file)) {
        m_error = errno;
        clearerr(m_file);
        return false;
    }

    return true;
}

bool File::close() {
    if (m_file) {
        if (fclose(m_file) == EOF) {
            m_error = errno;
        }
        m_file = nullptr;
    }
    return !m_error;
}

bool File::read(ByteBuffer& buffer) {
    if (buffer.capacity() == 0) {
        buffer.ensure_capacity(BUFSIZ);
    }

    auto current_size = buffer.size();
    auto nread = fread(buffer.data() + current_size, 1, buffer.capacity() - current_size, m_file);
    if (ferror(m_file)) {
        m_error = errno;
        clearerr(m_file);
        return false;
    }

    buffer.set_size(current_size + nread);
    return true;
}

bool File::write(ByteBuffer& buffer) {
    fwrite(buffer.data(), 1, buffer.size(), m_file);
    if (ferror(m_file)) {
        m_error = errno;
        clearerr(m_file);
        return false;
    }

    return true;
}

bool File::read_all_streamed(ByteBuffer& buffer, Function<bool(const ByteBuffer&)> callback) {
    for (;;) {
        bool read_result = read(buffer);
        if (!read_result) {
            return false;
        }

        bool callback_result = callback(buffer);
        if (!callback_result) {
            break;
        }

        if (buffer.size() == 0) {
            break;
        }
        buffer.set_size(0);
    }
    return true;
}
}
