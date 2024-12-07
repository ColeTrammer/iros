#pragma once

#include "di/container/path/prelude.h"
#include "di/function/prelude.h"
#include "di/util/prelude.h"
#include "di/vocab/error/prelude.h"
#include "dius/filesystem/file_status.h"
#include "dius/filesystem/file_type.h"
#include "dius/filesystem/query/prelude.h"

namespace dius::filesystem {

class DirectoryEntry {
public:
    DirectoryEntry() = default;

    DirectoryEntry(DirectoryEntry&&) = default;

    auto operator=(DirectoryEntry&&) -> DirectoryEntry& = default;

    constexpr auto path() const& -> di::Path const& { return m_path; }
    constexpr auto path() && -> di::Path&& { return di::move(m_path); }
    constexpr auto path_view() const -> di::PathView { return m_path.view(); }
    constexpr auto filename() const -> di::Optional<di::PathView> { return path_view().filename(); }

    constexpr operator di::Path const&() const& { return path(); }
    constexpr operator di::Path&&() && { return di::move(*this).path(); }
    constexpr operator di::PathView() const { return path_view(); }

    auto exists() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::exists(FileStatus(m_cached_type));
        }
        return filesystem::exists(path_view());
    }

    auto is_block_file() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_block_file(FileStatus(m_cached_type));
        }
        return filesystem::is_block_file(path_view());
    }

    auto is_character_file() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_character_file(FileStatus(m_cached_type));
        }
        return filesystem::is_character_file(path_view());
    }

    auto is_directory() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_directory(FileStatus(m_cached_type));
        }
        return filesystem::is_directory(path_view());
    }

    auto is_fifo() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_fifo(FileStatus(m_cached_type));
        }
        return filesystem::is_fifo(path_view());
    }

    auto is_other() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_other(FileStatus(m_cached_type));
        }
        return filesystem::is_other(path_view());
    }

    auto is_regular_file() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_regular_file(FileStatus(m_cached_type));
        }
        return filesystem::is_regular_file(path_view());
    }

    auto is_socket() const -> di::Result<bool> {
        if (has_cached_type()) {
            return filesystem::is_socket(FileStatus(m_cached_type));
        }
        return filesystem::is_socket(path_view());
    }

    auto is_symlink() const -> di::Result<bool> {
        if (m_cached_type != FileType::Unknown) {
            return filesystem::is_symlink(FileStatus(m_cached_type));
        }
        return filesystem::is_symlink(path_view());
    }

    auto file_size() const -> di::Result<umax> { return filesystem::file_size(path_view()); }
    auto hard_link_count() const -> di::Result<umax> { return filesystem::hard_link_count(path_view()); }

    auto status() const -> di::Result<FileStatus> { return filesystem::status(path_view()); }
    auto symlink_status() const -> di::Result<FileStatus> { return filesystem::symlink_status(path_view()); }

private:
    friend class DirectoryIterator;
    friend class RecursiveDirectoryIterator;

    constexpr friend auto operator==(DirectoryEntry const& a, DirectoryEntry const& b) -> bool {
        return a.path_view() == b.path_view();
    }
    constexpr friend auto operator<=>(DirectoryEntry const& a, DirectoryEntry const& b) -> di::strong_ordering {
        return a.path_view() <=> b.path_view();
    }

    explicit DirectoryEntry(di::Path&& path, FileType cached_type)
        : m_path(di::move(path)), m_cached_type(cached_type) {}

    auto is_non_symlink_directory() const -> di::Expected<bool, di::GenericCode> {
        if (m_cached_type != FileType::Unknown) {
            return m_cached_type == FileType::Directory;
        }
        // FIXME: this cast seems extremely dubious, and most likely
        //        is not performing the intended behavior.
        return di::Expected<bool, di::GenericCode>(is_directory());
    }

    constexpr auto has_cached_type() const -> bool {
        // NOTE: if the file type is a symlink, then we have to follow that symlink when
        //       answering all queries other than is_symlink(), which will require calling
        //       `status()`.
        return m_cached_type != FileType::Unknown && m_cached_type != FileType::Symlink;
    }

    template<di::concepts::Encoding Enc>
    constexpr friend auto tag_invoke(di::Tag<di::formatter_in_place>, di::InPlaceType<DirectoryEntry>,
                                     di::FormatParseContext<Enc>& context) {
        return di::format::formatter<di::PathView, Enc>(context) % [](di::concepts::CopyConstructible auto formatter) {
            return [=](di::FormatContext auto& context, DirectoryEntry const& a) {
                return formatter(context, a.path_view());
            };
        };
    }

    di::Path m_path;
    FileType m_cached_type { FileType::None };
};
}
