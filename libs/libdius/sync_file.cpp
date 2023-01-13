#include <dius/sync_file.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace dius {
di::Result<void> SyncFile::close() {
    if (m_owned == Owned::Yes && m_fd != -1) {
        (void) ::close(m_fd);
    }
    m_owned = Owned::No;
    m_fd = -1;
    return {};
}

di::Result<size_t> SyncFile::read(di::Span<di::Byte> data) const {
    auto result = ::read(m_fd, data.data(), data.size());
    (void) result;
    return di::to_unsigned(result);
}

di::Result<size_t> SyncFile::read(u64 offset, di::Span<di::Byte> data) const {
    auto result = ::pread(m_fd, data.data(), data.size(), offset);
    (void) result;
    return di::to_unsigned(result);
}

di::Result<size_t> SyncFile::write(di::Span<di::Byte const> data) const {
    auto result = ::write(m_fd, data.data(), data.size());
    (void) result;
    return data.size();
}

di::Result<size_t> SyncFile::write(u64 offset, di::Span<di::Byte const> data) const {
    auto result = ::pwrite(m_fd, data.data(), data.size(), offset);
    (void) result;
    return data.size();
}

di::Result<SyncFile> open_sync(di::PathView path, OpenMode open_mode, u16 create_mode) {
    auto open_mode_flags = [&] {
        switch (open_mode) {
            case OpenMode::Readonly:
                return O_RDONLY;
            case OpenMode::WriteNew:
                return O_WRONLY | O_EXCL | O_CREAT;
            case OpenMode::WriteClobber:
                return O_WRONLY | O_TRUNC | O_CREAT;
            case OpenMode::ReadWrite:
                return O_RDWR;
            case OpenMode::AppendOnly:
                return O_WRONLY | O_APPEND | O_CREAT;
            default:
                di::unreachable();
        }
    }();

    auto raw_data = path.data();
    char null_terminated_string[4096];
    ASSERT_LT(raw_data.size(), sizeof(null_terminated_string) - 1);

    memcpy(null_terminated_string, raw_data.data(), raw_data.size());
    null_terminated_string[raw_data.size()] = '\0';
    int fd = ::open(null_terminated_string, open_mode_flags, create_mode);
    if (fd < 0) {
        return di::Unexpected(di::BasicError::Invalid);
    }
    return SyncFile { SyncFile::Owned::Yes, fd };
}

di::Result<di::String> read_to_string(di::PathView path) {
    auto file = TRY(open_sync(path, OpenMode::Readonly));
    return di::read_to_string(file);
}
}