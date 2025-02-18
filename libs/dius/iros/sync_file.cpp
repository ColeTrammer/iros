#include "dius/sync_file.h"

#include <iris/uapi/open.h>

#include "di/function/prelude.h"
#include "di/math/prelude.h"
#include "dius/system/system_call.h"

namespace dius {
static auto sys_read(int fd, u64 offset, di::Span<byte> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::read, fd, data.data(), data.size(), offset);
}

static auto sys_write(int fd, u64 offset, di::Span<byte const> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::write, fd, data.data(), data.size(), offset);
}

static auto sys_close(int fd) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::close, fd) % di::into_void;
}

static auto sys_truncate(int fd, u64 size) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::truncate, fd, size) % di::into_void;
}

static auto sys_open(di::PathView path, iris::OpenMode mode) -> di::Expected<int, di::GenericCode> {
    return system::system_call<int>(system::Number::open, path.data().data(), path.data().size(), mode);
}

auto SyncFile::close() -> di::Expected<void, di::GenericCode> {
    auto owned = di::exchange(m_owned, Owned::No);
    auto fd = di::exchange(m_fd, -1);

    if (owned == Owned::Yes && fd != -1) {
        return sys_close(fd);
    }
    return {};
}

auto SyncFile::read_some(di::Span<byte> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_read(m_fd, di::NumericLimits<u64>::max, data);
}

auto SyncFile::read_some(u64 offset, di::Span<byte> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_read(m_fd, offset, data);
}

auto SyncFile::write_some(di::Span<byte const> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_write(m_fd, di::NumericLimits<u64>::max, data);
}

auto SyncFile::write_some(u64 offset, di::Span<byte const> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_write(m_fd, offset, data);
}

auto SyncFile::resize_file(u64 size) const -> di::Expected<void, di::GenericCode> {
    return sys_truncate(m_fd, size);
}

auto SyncFile::set_tty_window_size(tty::WindowSize) -> di::Expected<void, di::GenericCode> {
    return di::Unexpected(PosixError::OperationNotSupported);
}

auto SyncFile::get_tty_window_size() -> di::Expected<tty::WindowSize, di::GenericCode> {
    return di::Unexpected(PosixError::OperationNotSupported);
}

auto SyncFile::get_psuedo_terminal_path() -> di::Expected<di::Path, di::GenericCode> {
    return di::Unexpected(PosixError::OperationNotSupported);
}

auto SyncFile::enter_raw_mode() -> di::Expected<RawModeToken, di::GenericCode> {
    return di::Unexpected(PosixError::OperationNotSupported);
}

auto open_sync(di::PathView path, OpenMode mode, u16) -> di::Expected<SyncFile, di::GenericCode> {
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

auto open_psuedo_terminal_controller(OpenMode, tty::WindowSize) -> di::Expected<SyncFile, di::GenericCode> {
    return di::Unexpected(PosixError::OperationNotSupported);
}

auto open_tempory_file() -> di::Expected<SyncFile, di::GenericCode> {
    return di::Unexpected(PosixError::OperationNotSupported);
}
}
