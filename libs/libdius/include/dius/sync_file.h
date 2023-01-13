#pragma once

#include <di/prelude.h>

namespace dius {
class SyncFile {
public:
    enum class Owned { Yes, No };

    constexpr explicit SyncFile(Owned owned, int fd) : m_owned(owned), m_fd(fd) {}

    constexpr SyncFile(SyncFile&& other) : m_owned(di::exchange(other.m_owned, Owned::No)), m_fd(di::exchange(other.m_fd, -1)) {}

    ~SyncFile() { (void) this->close(); }

    SyncFile& operator=(SyncFile&& other) {
        (void) this->close();
        m_owned = di::exchange(other.m_owned, Owned::No);
        m_fd = di::exchange(other.m_fd, -1);
        return *this;
    }

    constexpr bool valid() const { return m_fd != -1; }
    constexpr explicit operator bool() const { return valid(); }

    constexpr int file_descriptor() const { return m_fd; }

    di::Result<void> close();

    di::Result<u64> read(u64 offset, di::Span<di::Byte>) const;
    di::Result<u64> read(di::Span<di::Byte>) const;
    di::Result<u64> write(u64 offset, di::Span<di::Byte const>) const;
    di::Result<u64> write(di::Span<di::Byte const>) const;

    di::Result<void> flush() const { return {}; }

private:
    Owned m_owned { Owned::No };
    int m_fd { -1 };
};

enum class OpenMode { Readonly, WriteNew, WriteClobber, ReadWrite, AppendOnly };

di::Result<SyncFile> open_sync(di::PathView path, OpenMode open_mode, u16 create_mode = 0666);
di::Result<di::String> read_to_string(di::PathView path);

inline auto stdin = SyncFile { SyncFile::Owned::No, 0 };
inline auto stdout = SyncFile { SyncFile::Owned::No, 1 };
inline auto stderr = SyncFile { SyncFile::Owned::No, 2 };
}