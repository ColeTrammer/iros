#pragma once

#include "di/container/path/prelude.h"
#include "di/util/prelude.h"
#include "di/vocab/error/prelude.h"
#include "dius/error.h"
#include "dius/filesystem/directory_entry.h"
#include "dius/filesystem/directory_options.h"
#include "dius/sync_file.h"

namespace dius::filesystem {
class DirectoryIterator
    : public di::container::IteratorBase<DirectoryIterator, di::InputIteratorTag,
                                         di::Expected<DirectoryEntry, di::GenericCode>, i64>
    , public di::meta::EnableBorrowedContainer<DirectoryIterator>
    , public di::meta::EnableView<DirectoryIterator> {
    friend class RecursiveDirectoryIterator;

public:
    static auto create(di::Path path, DirectoryOptions options = DirectoryOptions::None)
        -> di::Expected<DirectoryIterator, di::GenericCode>;

    DirectoryIterator() = default;

    DirectoryIterator(DirectoryIterator const&) = delete;
    DirectoryIterator(DirectoryIterator&&) = default;

    auto operator=(DirectoryIterator const&) -> DirectoryIterator& = delete;
    auto operator=(DirectoryIterator&&) -> DirectoryIterator& = default;

    auto operator*() const -> di::Expected<DirectoryEntry const&, di::GenericCode> {
        return m_current.transform([](auto const& value) {
            return di::cref(value);
        });
    }

    auto begin() -> DirectoryIterator { return di::move(*this); }
    auto end() const -> DirectoryIterator { return {}; }

    void advance_one();

private:
    void advance();

    explicit DirectoryIterator(di::Path&& path, di::Vector<di::Byte>&& buffer, SyncFile&& directory_handle)
        : m_path(di::move(path))
        , m_buffer(di::move(buffer))
        , m_directory_handle(di::move(directory_handle))
        , m_at_end(false) {}

    constexpr friend auto operator==(DirectoryIterator const& a, DirectoryIterator const& b) -> bool {
        return a.m_at_end == b.m_at_end;
    }

    di::Path m_path;
    di::Vector<di::Byte> m_buffer;
    SyncFile m_directory_handle;
    di::Expected<DirectoryEntry, di::GenericCode> m_current { di::unexpect, PosixError::Success };
    usize m_current_offset { 0 };
    bool m_at_end { true };
};
}
