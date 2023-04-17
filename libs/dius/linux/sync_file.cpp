#include <dius/sync_file.h>
#include <dius/system/system_call.h>

namespace dius {
di::Expected<usize, PosixCode> sys_read(int fd, di::Span<byte> data) {
    return system::system_call<usize>(system::Number::read, fd, data.data(), data.size());
}

di::Expected<usize, PosixCode> sys_write(int fd, di::Span<byte const> data) {
    return system::system_call<usize>(system::Number::write, fd, data.data(), data.size());
}

di::Expected<usize, PosixCode> sys_pread(int fd, u64 offset, di::Span<byte> data) {
    return system::system_call<usize>(system::Number::pread, fd, data.data(), data.size(), offset);
}

di::Expected<usize, PosixCode> sys_pwrite(int fd, u64 offset, di::Span<byte const> data) {
    return system::system_call<usize>(system::Number::pwrite, fd, data.data(), data.size(), offset);
}

di::Expected<void, PosixCode> sys_close(int fd) {
    return system::system_call<int>(system::Number::close, fd) % di::into_void;
}

di::Expected<int, PosixCode> sys_open(di::PathView path, int flags, u16 create_mode) {
    auto raw_data = path.data();
    char null_terminated_string[4097];
    ASSERT_LT(raw_data.size(), sizeof(null_terminated_string) - 1);

    di::copy(raw_data, null_terminated_string);
    null_terminated_string[raw_data.size()] = '\0';

    return system::system_call<int>(system::Number::openat, AT_FDCWD, null_terminated_string, flags, create_mode);
}

di::Expected<void, PosixCode> sys_ftruncate(int fd, u64 size) {
    return system::system_call<int>(system::Number::ftruncate, fd, size) % di::into_void;
}

di::Expected<byte*, PosixCode> sys_mmap(void* addr, usize length, Protection prot, MapFlags flags, int fd, u64 offset) {
    return system::system_call<byte*>(system::Number::mmap, addr, length, prot, flags, fd, offset);
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
    return sys_read(m_fd, data);
}

di::Expected<usize, PosixCode> SyncFile::read_some(u64 offset, di::Span<byte> data) const {
    return sys_pread(m_fd, offset, data);
}

di::Expected<usize, PosixCode> SyncFile::write_some(di::Span<byte const> data) const {
    return sys_write(m_fd, data);
}

di::Expected<usize, PosixCode> SyncFile::write_some(u64 offset, di::Span<byte const> data) const {
    return sys_pwrite(m_fd, offset, data);
}

di::Expected<void, PosixCode> SyncFile::resize_file(u64 new_size) const {
    return sys_ftruncate(file_descriptor(), new_size);
}

di::Expected<MemoryRegion, PosixCode> SyncFile::map(u64 offset, usize size, Protection protection,
                                                    MapFlags flags) const {
    auto* base = TRY(sys_mmap(nullptr, size, protection, flags, m_fd, offset));
    return MemoryRegion(di::Span { base, size });
}

di::Expected<SyncFile, PosixCode> open_sync(di::PathView path, OpenMode open_mode, u16 create_mode) {
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

di::Expected<SyncFile, PosixCode> open_tempory_file() {
    auto fd = TRY(sys_open("/tmp"_pv, O_TMPFILE | O_RDWR, 0666));
    return SyncFile { SyncFile::Owned::Yes, fd };
}
}
