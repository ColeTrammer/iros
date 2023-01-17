#include <dius/error.h>
#include <dius/sync_file.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace dius {
di::Result<void> SyncFile::close() {
    auto owned = di::exchange(m_owned, Owned::No);
    auto fd = di::exchange(m_fd, -1);

    if (owned == Owned::Yes && fd != -1) {
        if (::close(fd)) {
            return di::Unexpected(PosixError(errno));
        }
    }
    return {};
}

di::Result<size_t> SyncFile::read(di::Span<di::Byte> data) const {
    auto result = ::read(m_fd, data.data(), data.size());
    if (result < 0) {
        return di::Unexpected(PosixError(errno));
    }
    return di::to_unsigned(result);
}

di::Result<size_t> SyncFile::read(u64 offset, di::Span<di::Byte> data) const {
    auto result = ::pread(m_fd, data.data(), data.size(), offset);
    if (result < 0) {
        return di::Unexpected(PosixError(errno));
    }
    return di::to_unsigned(result);
}

di::Result<size_t> SyncFile::write(di::Span<di::Byte const> data) const {
    auto result = ::write(m_fd, data.data(), data.size());
    if (result < 0) {
        return di::Unexpected(PosixError(errno));
    }
    return data.size();
}

di::Result<size_t> SyncFile::write(u64 offset, di::Span<di::Byte const> data) const {
    auto result = ::pwrite(m_fd, data.data(), data.size(), offset);
    if (result < 0) {
        return di::Unexpected(PosixError(errno));
    }
    return data.size();
}

di::Result<MemoryRegion> SyncFile::map(u64 offset, size_t size, Protection protection, MapFlags flags) const {
    auto result = ::mmap(nullptr, size, di::to_underlying(protection), di::to_underlying(flags), m_fd, offset);
    if (result == reinterpret_cast<void*>(-1)) {
        return di::Unexpected(PosixError(errno));
    }
    return MemoryRegion(di::Span { reinterpret_cast<di::Byte*>(result), size });
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
        return di::Unexpected(PosixError(errno));
    }
    return SyncFile { SyncFile::Owned::Yes, fd };
}

di::Result<di::String> read_to_string(di::PathView path) {
    auto file = TRY(open_sync(path, OpenMode::Readonly));
    return di::read_to_string(file);
}
}