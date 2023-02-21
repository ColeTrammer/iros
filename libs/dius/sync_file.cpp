#include <dius/prelude.h>

namespace dius {
di::Result<size_t> sys_read(int fd, di::Span<di::Byte> data) {
    return system::system_call<size_t>(system::Number::read, fd, data.data(), data.size());
}

di::Result<size_t> sys_write(int fd, di::Span<di::Byte const> data) {
    return system::system_call<size_t>(system::Number::write, fd, data.data(), data.size());
}

di::Result<size_t> sys_pread(int fd, u64 offset, di::Span<di::Byte> data) {
    return system::system_call<size_t>(system::Number::pread, fd, data.data(), data.size(), offset);
}

di::Result<size_t> sys_pwrite(int fd, u64 offset, di::Span<di::Byte const> data) {
    return system::system_call<size_t>(system::Number::pwrite, fd, data.data(), data.size(), offset);
}

di::Result<void> sys_close(int fd) {
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

di::Result<void> sys_ftruncate(int fd, u64 size) {
    return system::system_call<int>(system::Number::ftruncate, fd, size) % di::into_void;
}

di::Result<di::Byte*> sys_mmap(void* addr, size_t length, Protection prot, MapFlags flags, int fd, u64 offset) {
    return system::system_call<di::Byte*>(system::Number::mmap, addr, length, prot, flags, fd, offset);
}

di::Result<void> SyncFile::close() {
    auto owned = di::exchange(m_owned, Owned::No);
    auto fd = di::exchange(m_fd, -1);

    if (owned == Owned::Yes && fd != -1) {
        return sys_close(fd);
    }
    return {};
}

di::Result<size_t> SyncFile::read_some(di::Span<di::Byte> data) const {
    return sys_read(m_fd, data);
}

di::Result<size_t> SyncFile::read_some(u64 offset, di::Span<di::Byte> data) const {
    return sys_pread(m_fd, offset, data);
}

di::Result<size_t> SyncFile::write_some(di::Span<di::Byte const> data) const {
    return sys_write(m_fd, data);
}

di::Result<size_t> SyncFile::write_some(u64 offset, di::Span<di::Byte const> data) const {
    return sys_pwrite(m_fd, offset, data);
}

di::Result<void> SyncFile::read_exactly(u64 offset, di::Span<di::Byte> data) const {
    while (!data.empty()) {
        auto nread = TRY(read_some(offset, data));
        if (nread == 0) {
            return di::Unexpected(PosixError::IoError);
        }
        data = *data.subspan(nread);
        offset += nread;
    }
    return {};
}

di::Result<void> SyncFile::read_exactly(di::Span<di::Byte> data) const {
    while (!data.empty()) {
        auto nread = TRY(read_some(data));
        if (nread == 0) {
            return di::Unexpected(PosixError::IoError);
        }
        data = *data.subspan(nread);
    }
    return {};
}

di::Result<void> SyncFile::write_exactly(u64 offset, di::Span<di::Byte const> data) const {
    while (!data.empty()) {
        auto nwritten = TRY(write_some(offset, data));
        if (nwritten == 0) {
            return di::Unexpected(PosixError::IoError);
        }
        data = *data.subspan(nwritten);
        offset += nwritten;
    }
    return {};
}

di::Result<void> SyncFile::write_exactly(di::Span<di::Byte const> data) const {
    while (!data.empty()) {
        auto nwritten = TRY(write_some(data));
        if (nwritten == 0) {
            return di::Unexpected(PosixError::IoError);
        }
        data = *data.subspan(nwritten);
    }
    return {};
}

di::Result<void> SyncFile::resize_file(u64 new_size) const {
    return sys_ftruncate(file_descriptor(), new_size);
}

di::Result<MemoryRegion> SyncFile::map(u64 offset, size_t size, Protection protection, MapFlags flags) const {
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
            default:
                di::unreachable();
        }
    }();

    auto fd = TRY(sys_open(path, open_mode_flags, create_mode));
    return SyncFile { SyncFile::Owned::Yes, fd };
}

di::Result<di::String> read_to_string(di::PathView path) {
    auto file = TRY(open_sync(path, OpenMode::Readonly));
    return di::read_to_string(file);
}
}
