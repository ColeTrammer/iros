#include <errno.h>
#include <ext/new_file.h>
#include <ext/system.h>
#include <fcntl.h>
#include <liim/byte_buffer.h>
#include <liim/container/path.h>
#include <liim/span.h>
#include <liim/string_view.h>
#include <liim/try.h>
#include <liim/tuple.h>
#include <unistd.h>

namespace Ext {
#define ENSURE_IS_OPEN()                          \
    do {                                          \
        if (!is_open()) {                         \
            return Err(make_system_error(EBADF)); \
        }                                         \
    } while (0)

System::Result<NewFile> NewFile::create(PathView path, OpenMode open_mode, mode_t create_mode) {
    auto [open_mode_flags, open_mode_string] = [&]() -> Tuple<System::OpenMode, const char*> {
        switch (open_mode) {
            case OpenMode::Readonly:
                return { System::OpenMode::ReadOnly, "r" };
            case OpenMode::WriteNew:
                return { System::OpenMode::WriteOnly | System::OpenMode::Create | System::OpenMode::Exclusive, "w" };
            case OpenMode::WriteClobber:
                return { System::OpenMode::WriteOnly | System::OpenMode::Create | System::OpenMode::Truncate, "w" };
            case OpenMode::ReadWrite:
                return { System::OpenMode::ReadWrite, "r+" };
            case OpenMode::AppendOnly:
                return { System::OpenMode::WriteOnly | System::OpenMode::Create | System::OpenMode::Append, "a" };
            default:
                assert(false);
        }
    }();

    int fd = TRY(System::open(path, open_mode_flags, create_mode));
    FILE* file = fdopen(fd, open_mode_string);
    if (!file) {
        return Err(make_system_error_from_errno());
    }
    return NewFile(file);
}

NewFile::NewFile(FILE* file) : m_file(file) {}

NewFile::~NewFile() {
    if (m_file) {
        close();
    }
}

Option<int> NewFile::fd() const {
    if (!is_open()) {
        return None {};
    }
    return fileno(m_file);
}

System::Result<void> NewFile::write(const ByteBuffer& buffer) {
    return write(buffer.span());
}

System::Result<void> NewFile::write(StringView buffer) {
    return write({ reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size() });
}

System::Result<void> NewFile::write(Span<const uint8_t> buffer) {
    ENSURE_IS_OPEN();

    while (!buffer.empty()) {
        auto written = TRY(write_allow_partial(buffer));
        buffer = buffer.subspan(written);
    }
    return {};
}

System::Result<size_t> NewFile::write_allow_partial(const ByteBuffer& buffer) {
    return this->write_allow_partial(buffer.span());
}

System::Result<size_t> NewFile::write_allow_partial(StringView buffer) {
    return this->write_allow_partial({ reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size() });
}

System::Result<size_t> NewFile::write_allow_partial(Span<const uint8_t> buffer) {
    ENSURE_IS_OPEN();

    if (buffer.empty()) {
        return 0;
    }

    size_t written = fwrite(buffer.data(), 1, buffer.size(), m_file);
    if (written == 0) {
        return Err(make_system_error_from_errno());
    }
    return written;
}

System::Result<void> NewFile::close() {
    ENSURE_IS_OPEN();

    if (fclose(m_file) == EOF) {
        return Err(make_system_error_from_errno());
    }
    return {};
}

System::Result<ByteBuffer> NewFile::map(System::Protection protection, System::MapFlags flags) const {
    ENSURE_IS_OPEN();

    auto fd = *this->fd();
    auto stat = TRY(System::fstat(fd));
    auto length = stat.st_size;
    auto* address = TRY(System::mmap(nullptr, length, protection, flags, fd, 0));
    return ByteBuffer::create_from_memory_mapping(static_cast<uint8_t*>(address), length);
}
}
