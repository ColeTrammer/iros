#pragma once

#include <dius/filesystem/file_type.h>
#include <dius/filesystem/perms.h>

namespace dius::filesystem {
class FileStatus {
public:
    constexpr FileStatus() : FileStatus(FileType::None) {}

    FileStatus(FileStatus const&) = default;
    FileStatus(FileStatus&&) = default;

    constexpr explicit FileStatus(FileType type, Perms permissions = Perms::Unknown)
        : m_type(type), m_permsissions(permissions) {}

    auto operator=(FileStatus const&) -> FileStatus& = default;
    auto operator=(FileStatus&&) -> FileStatus& = default;

    constexpr auto type() const -> FileType { return m_type; }
    constexpr auto permissions() const -> Perms { return m_permsissions; }

private:
    constexpr friend auto operator==(FileStatus const& a, FileStatus const& b) -> bool {
        return a.type() == b.type() && a.permissions() == b.permissions();
    }

    FileType m_type;
    Perms m_permsissions;
};
}
