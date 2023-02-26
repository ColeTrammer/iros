#include <dius/prelude.h>

namespace dius {
di::Result<usize> sys_read(int fd, u64 offset, di::Span<di::Byte> data) {
    return system::system_call<usize>(system::Number::read, fd, data.data(), data.size(), offset);
}

di::Result<usize> sys_write(int fd, u64 offset, di::Span<di::Byte const> data) {
    return system::system_call<usize>(system::Number::write, fd, data.data(), data.size(), offset);
}

di::Result<void> sys_close(int fd) {
    return system::system_call<int>(system::Number::close, fd) % di::into_void;
}

di::Expected<int, PosixCode> sys_open(di::PathView path) {
    return system::system_call<int>(system::Number::open, path.data().data(), path.data().size());
}

di::Result<void> SyncFile::close() {
    auto owned = di::exchange(m_owned, Owned::No);
    auto fd = di::exchange(m_fd, -1);

    if (owned == Owned::Yes && fd != -1) {
        return sys_close(fd);
    }
    return {};
}

di::Result<usize> SyncFile::read_some(di::Span<di::Byte> data) const {
    return sys_read(m_fd, di::NumericLimits<u64>::max, data);
}

di::Result<usize> SyncFile::read_some(u64 offset, di::Span<di::Byte> data) const {
    return sys_read(m_fd, offset, data);
}

di::Result<usize> SyncFile::write_some(di::Span<di::Byte const> data) const {
    return sys_write(m_fd, di::NumericLimits<u64>::max, data);
}

di::Result<usize> SyncFile::write_some(u64 offset, di::Span<di::Byte const> data) const {
    return sys_write(m_fd, offset, data);
}

di::Expected<SyncFile, PosixCode> open_sync(di::PathView path, OpenMode, u16) {
    auto fd = TRY(sys_open(path));
    return SyncFile { SyncFile::Owned::Yes, fd };
}
}
