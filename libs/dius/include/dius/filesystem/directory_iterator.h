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
public:
    static di::Result<DirectoryIterator> create(di::Path path, DirectoryOptions options = DirectoryOptions::None);

    DirectoryIterator(DirectoryIterator&&) = default;

    DirectoryIterator& operator=(DirectoryIterator&&) = default;

    di::Expected<DirectoryEntry const&, PosixCode> operator*() const {
        return m_current.transform([](auto const& value) {
            return di::cref(value);
        });
    }
    di::Expected<DirectoryEntry, PosixCode>* operator->() { return di::addressof(m_current); }

    DirectoryIterator begin() && { return di::move(*this); }
    constexpr di::DefaultSentinel end() const { return {}; }

    void advance_one();

private:
    explicit DirectoryIterator(di::Path&& path, SyncFile&& directory_handle, DirectoryOptions options)
        : m_path(di::move(path)), m_directory_handle(di::move(directory_handle)), m_options(options), m_at_end(false) {}

    constexpr friend bool operator==(DirectoryIterator const& a, DirectoryIterator const& b) {
        return a.m_at_end == b.m_at_end;
    }

    di::Path m_path;
    SyncFile m_directory_handle;
    di::Expected<DirectoryEntry, PosixCode> m_current { di::unexpect, PosixError::Success };
    DirectoryOptions m_options { DirectoryOptions::None };
    bool m_at_end { true };
};
}