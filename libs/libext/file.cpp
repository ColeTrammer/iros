#include <errno.h>
#include <ext/file.h>

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
    close();
}

Maybe<ByteBuffer> read_all() {
    return {};
}

bool File::close() {
    if (m_file) {
        if (fclose(m_file) == EOF) {
            m_error = errno;
        }
        m_file = nullptr;
    }
    return m_error;
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
    auto current_size = buffer.size();
    auto nwritten = fwrite(buffer.data(), 1, buffer.size(), m_file);
    if (ferror(m_file)) {
        m_error = errno;
        clearerr(m_file);
        return false;
    }

    buffer.set_size(current_size + nwritten);
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
