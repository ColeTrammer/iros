#include <dius/prelude.h>

namespace dius {
di::Expected<usize, PosixCode> sys_read(int fd, u64 offset, di::Span<byte> data) {
    return system::system_call<usize>(system::Number::read, fd, data.data(), data.size(), offset);
}

di::Expected<usize, PosixCode> sys_write(int fd, u64 offset, di::Span<byte const> data) {
    return system::system_call<usize>(system::Number::write, fd, data.data(), data.size(), offset);
}

di::Expected<void, PosixCode> sys_close(int fd) {
    return system::system_call<int>(system::Number::close, fd) % di::into_void;
}

di::Expected<int, PosixCode> sys_open(di::PathView path) {
    return system::system_call<int>(system::Number::open, path.data().data(), path.data().size());
}

di::Expected<void, PosixCode> SyncFile::close() {
    auto owned = di::exchange(m_owned, Owned::No);
    auto fd = di::exchange(m_fd, -1);

    if (owned == Owned::Yes && fd != -1) {
        return sys_close(fd);
    }
    return {};
}

di::Expected<usize, PosixCode> SyncFile::read_some(di::Span<byte> data) const {
    return sys_read(m_fd, di::NumericLimits<u64>::max, data);
}

di::Expected<usize, PosixCode> SyncFile::read_some(u64 offset, di::Span<byte> data) const {
    return sys_read(m_fd, offset, data);
}

di::Expected<usize, PosixCode> SyncFile::write_some(di::Span<byte const> data) const {
    return sys_write(m_fd, di::NumericLimits<u64>::max, data);
}

di::Expected<usize, PosixCode> SyncFile::write_some(u64 offset, di::Span<byte const> data) const {
    return sys_write(m_fd, offset, data);
}

di::Expected<SyncFile, PosixCode> open_sync(di::PathView path, OpenMode, u16) {
    auto fd = TRY(sys_open(path));
    return SyncFile { SyncFile::Owned::Yes, fd };
}
}
