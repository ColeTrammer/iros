#pragma once

#include <di/prelude.h>
#include <dius/filesystem/file_status.h>
#include <dius/filesystem/file_type.h>
#include <dius/filesystem/query/prelude.h>

namespace dius::filesystem {

class DirectoryEntry {
public:
    DirectoryEntry() = default;

    DirectoryEntry(DirectoryEntry&&) = default;

    DirectoryEntry& operator=(DirectoryEntry&&) = default;

    constexpr di::Path const& path() const& { return m_path; }
    constexpr di::Path&& path() && { return di::move(m_path); }
    constexpr di::PathView path_view() const { return m_path.view(); }
    constexpr di::Optional<di::PathView> filename() const { return path_view().filename(); }

    constexpr operator di::Path const&() const& { return path(); }
    constexpr operator di::Path&&() && { return di::move(*this).path(); }
    constexpr operator di::PathView() const { return path_view(); }

    di::Result<bool> exists() const {
        if (has_cached_type()) {
            return filesystem::exists(FileStatus(m_cached_type));
        }
        return filesystem::exists(path_view());
    }

    di::Result<bool> is_block_file() const {
        if (has_cached_type()) {
            return filesystem::is_block_file(FileStatus(m_cached_type));
        }
        return filesystem::is_block_file(path_view());
    }

    di::Result<bool> is_character_file() const {
        if (has_cached_type()) {
            return filesystem::is_character_file(FileStatus(m_cached_type));
        }
        return filesystem::is_character_file(path_view());
    }

    di::Result<bool> is_directory() const {
        if (has_cached_type()) {
            return filesystem::is_directory(FileStatus(m_cached_type));
        }
        return filesystem::is_directory(path_view());
    }

    di::Result<bool> is_fifo() const {
        if (has_cached_type()) {
            return filesystem::is_fifo(FileStatus(m_cached_type));
        }
        return filesystem::is_fifo(path_view());
    }

    di::Result<bool> is_other() const {
        if (has_cached_type()) {
            return filesystem::is_other(FileStatus(m_cached_type));
        }
        return filesystem::is_other(path_view());
    }

    di::Result<bool> is_regular_file() const {
        if (has_cached_type()) {
            return filesystem::is_regular_file(FileStatus(m_cached_type));
        }
        return filesystem::is_regular_file(path_view());
    }

    di::Result<bool> is_socket() const {
        if (has_cached_type()) {
            return filesystem::is_socket(FileStatus(m_cached_type));
        }
        return filesystem::is_socket(path_view());
    }

    di::Result<bool> is_symlink() const {
        if (m_cached_type != FileType::Unknown) {
            return filesystem::is_symlink(FileStatus(m_cached_type));
        }
        return filesystem::is_symlink(path_view());
    }

    di::Result<umax> file_size() const { return filesystem::file_size(path_view()); }
    di::Result<umax> hard_link_count() const { return filesystem::hard_link_count(path_view()); }

    di::Result<FileStatus> status() const { return filesystem::status(path_view()); }
    di::Result<FileStatus> symlink_status() const { return filesystem::symlink_status(path_view()); }

private:
    friend struct DirectoryIterator;

    constexpr friend bool operator==(DirectoryEntry const& a, DirectoryEntry const& b) {
        return a.path_view() == b.path_view();
    }
    constexpr friend di::strong_ordering operator<=>(DirectoryEntry const& a, DirectoryEntry const& b) {
        return a.path_view() <=> b.path_view();
    }

    explicit DirectoryEntry(di::Path&& path, FileType cached_type)
        : m_path(di::move(path)), m_cached_type(cached_type) {}

    constexpr bool has_cached_type() const {
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