#include <dius/sync_file.h>

#include <di/assert/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <di/function/prelude.h>
#include <dius/system/system_call.h>

namespace dius {
auto sys_read(int fd, di::Span<byte> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::read, fd, data.data(), data.size());
}

auto sys_write(int fd, di::Span<byte const> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::write, fd, data.data(), data.size());
}

auto sys_pread(int fd, u64 offset, di::Span<byte> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::pread, fd, data.data(), data.size(), offset);
}

auto sys_pwrite(int fd, u64 offset, di::Span<byte const> data) -> di::Expected<usize, di::GenericCode> {
    return system::system_call<usize>(system::Number::pwrite, fd, data.data(), data.size(), offset);
}

auto sys_close(int fd) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::close, fd) % di::into_void;
}

auto sys_open(di::PathView path, int flags, u16 create_mode) -> di::Expected<int, di::GenericCode> {
    auto raw_data = path.data();
    char null_terminated_string[4097];
    ASSERT_LT(raw_data.size(), sizeof(null_terminated_string) - 1);

    di::copy(raw_data, null_terminated_string);
    null_terminated_string[raw_data.size()] = '\0';

    return system::system_call<int>(system::Number::openat, AT_FDCWD, null_terminated_string, flags, create_mode);
}

auto sys_ftruncate(int fd, u64 size) -> di::Expected<void, di::GenericCode> {
    return system::system_call<int>(system::Number::ftruncate, fd, size) % di::into_void;
}

auto sys_mmap(void* addr, usize length, Protection prot, MapFlags flags, int fd, u64 offset)
    -> di::Expected<byte*, di::GenericCode> {
    return system::system_call<byte*>(system::Number::mmap, addr, length, prot, flags, fd, offset);
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
    return sys_read(m_fd, data);
}

auto SyncFile::read_some(u64 offset, di::Span<byte> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_pread(m_fd, offset, data);
}

auto SyncFile::write_some(di::Span<byte const> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_write(m_fd, data);
}

auto SyncFile::write_some(u64 offset, di::Span<byte const> data) const -> di::Expected<usize, di::GenericCode> {
    return sys_pwrite(m_fd, offset, data);
}

auto SyncFile::resize_file(u64 new_size) const -> di::Expected<void, di::GenericCode> {
    return sys_ftruncate(file_descriptor(), new_size);
}

auto SyncFile::map(u64 offset, usize size, Protection protection, MapFlags flags) const
    -> di::Expected<MemoryRegion, di::GenericCode> {
    auto* base = TRY(sys_mmap(nullptr, size, protection, flags, m_fd, offset));
    return MemoryRegion(di::Span { base, size });
}

auto open_sync(di::PathView path, OpenMode open_mode, u16 create_mode) -> di::Expected<SyncFile, di::GenericCode> {
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
            case OpenMode::ReadWriteClobber:
                return O_RDWR | O_TRUNC | O_CREAT;
            case OpenMode::AppendReadWrite:
                return O_RDWR | O_APPEND | O_CREAT;
            default:
                di::unreachable();
        }
    }();

    auto fd = TRY(sys_open(path, open_mode_flags, create_mode));
    return SyncFile { SyncFile::Owned::Yes, fd };
}

auto open_tempory_file() -> di::Expected<SyncFile, di::GenericCode> {
    auto fd = TRY(sys_open("/tmp"_pv, O_TMPFILE | O_RDWR, 0666));
    return SyncFile { SyncFile::Owned::Yes, fd };
}
}
