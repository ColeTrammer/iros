#include "dius/sync_file.h"

#include "di/io/prelude.h"

namespace dius {
auto SyncFile::read_exactly(u64 offset, di::Span<byte> data) const -> di::Expected<void, di::GenericCode> {
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

auto SyncFile::read_exactly(di::Span<byte> data) const -> di::Expected<void, di::GenericCode> {
    while (!data.empty()) {
        auto nread = TRY(read_some(data));
        if (nread == 0) {
            return di::Unexpected(PosixError::IoError);
        }
        data = *data.subspan(nread);
    }
    return {};
}

auto SyncFile::write_exactly(u64 offset, di::Span<byte const> data) const -> di::Expected<void, di::GenericCode> {
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

auto SyncFile::write_exactly(di::Span<byte const> data) const -> di::Expected<void, di::GenericCode> {
    while (!data.empty()) {
        auto nwritten = TRY(write_some(data));
        if (nwritten == 0) {
            return di::Unexpected(PosixError::IoError);
        }
        data = *data.subspan(nwritten);
    }
    return {};
}

auto read_to_string(di::PathView path) -> di::Result<di::String> {
    auto file = TRY(open_sync(path, OpenMode::Readonly));
    return di::read_to_string(file);
}
}
