#include <dius/prelude.h>

namespace dius {
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

di::Result<di::String> read_to_string(di::PathView path) {
    auto file = TRY(open_sync(path, OpenMode::Readonly));
    return di::read_to_string(file);
}
}
