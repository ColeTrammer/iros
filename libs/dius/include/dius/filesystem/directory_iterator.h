#pragma once

#include <di/prelude.h>
#include <dius/error.h>
#include <dius/filesystem/directory_entry.h>
#include <dius/filesystem/directory_options.h>
#include <dius/sync_file.h>

namespace dius::filesystem {
class DirectoryIterator
    : public di::container::IteratorBase<DirectoryIterator, di::InputIteratorTag,
                                         di::Expected<DirectoryEntry, PosixCode>, i64>
    , public di::meta::EnableBorrowedContainer<DirectoryIterator>
    , public di::meta::EnableView<DirectoryIterator> {
    friend class RecursiveDirectoryIterator;

public:
    static di::Expected<DirectoryIterator, PosixCode> create(di::Path path,
                                                             DirectoryOptions options = DirectoryOptions::None);

    DirectoryIterator() = default;

    DirectoryIterator(DirectoryIterator const&) = delete;
    DirectoryIterator(DirectoryIterator&&) = default;

    DirectoryIterator& operator=(DirectoryIterator const&) = delete;
    DirectoryIterator& operator=(DirectoryIterator&&) = default;

    di::Expected<DirectoryEntry const&, PosixCode> operator*() const {
        return m_current.transform([](auto const& value) {
            return di::cref(value);
        });
    }

    DirectoryIterator begin() { return di::move(*this); }
    DirectoryIterator end() const { return {}; }

    void advance_one();

private:
    void advance();

    explicit DirectoryIterator(di::Path&& path, di::Vector<di::Byte>&& buffer, SyncFile&& directory_handle)
        : m_path(di::move(path))
        , m_buffer(di::move(buffer))
        , m_directory_handle(di::move(directory_handle))
        , m_at_end(false) {}

    constexpr friend bool operator==(DirectoryIterator const& a, DirectoryIterator const& b) {
        return a.m_at_end == b.m_at_end;
    }

    di::Path m_path;
    di::Vector<di::Byte> m_buffer;
    SyncFile m_directory_handle;
    di::Expected<DirectoryEntry, PosixCode> m_current { di::unexpect, PosixError::Success };
    usize m_current_offset { 0 };
    bool m_at_end { true };
};
}
