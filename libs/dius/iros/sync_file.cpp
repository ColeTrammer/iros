#include <di/function/prelude.h>
#include <di/math/prelude.h>
#include <dius/sync_file.h>
#include <dius/system/system_call.h>
#include <iris/uapi/open.h>

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

di::Expected<void, PosixCode> sys_truncate(int fd, u64 size) {
    return system::system_call<int>(system::Number::truncate, fd, size) % di::into_void;
}

di::Expected<int, PosixCode> sys_open(di::PathView path, iris::OpenMode mode) {
    return system::system_call<int>(system::Number::open, path.data().data(), path.data().size(), mode);
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

di::Expected<void, PosixCode> SyncFile::resize_file(u64 size) const {
    return sys_truncate(m_fd, size);
}

di::Expected<SyncFile, PosixCode> open_sync(di::PathView path, OpenMode mode, u16) {
    auto iris_mode = [&] {
        switch (mode) {
            case OpenMode::WriteNew:
            case OpenMode::WriteClobber:
            case OpenMode::ReadWriteClobber:
            case OpenMode::AppendReadWrite:
            case OpenMode::AppendOnly:
                return iris::OpenMode::Create;
            default:
                return iris::OpenMode::None;
        }
        return iris::OpenMode::Create;
    }();

    auto fd = TRY(sys_open(path, iris_mode));
    return SyncFile { SyncFile::Owned::Yes, fd };
}

di::Expected<SyncFile, PosixCode> open_tempory_file() {
    return di::Unexpected(PosixError::OperationNotSupported);
}
}
