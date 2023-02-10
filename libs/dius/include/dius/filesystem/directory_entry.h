#pragma once

#include <di/prelude.h>
#include <dius/filesystem/file_status.h>
#include <dius/filesystem/file_type.h>

namespace dius::filesystem {

class DirectoryEntry {
public:
    DirectoryEntry() = default;

    DirectoryEntry(DirectoryEntry&&) = default;

    DirectoryEntry& operator=(DirectoryEntry&&) = default;

    constexpr di::Path const& path() const& { return m_path; }
    constexpr di::Path&& path() && { return di::move(m_path); }
    constexpr di::PathView path_view() const { return m_path.view(); }

    constexpr operator di::Path const&() const& { return path(); }
    constexpr operator di::Path&&() && { return di::move(*this).path(); }
    constexpr operator di::PathView() const { return path_view(); }

private:
    friend struct DirectoryIterator;

    explicit DirectoryEntry(di::Path&& path, FileType cached_type)
        : m_path(di::move(path)), m_cached_type(cached_type) {}

    di::Path m_path;
    FileType m_cached_type { FileType::None };
};
}