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

    FileStatus& operator=(FileStatus const&) = default;
    FileStatus& operator=(FileStatus&&) = default;

    constexpr FileType type() const { return m_type; }
    constexpr Perms permissions() const { return m_permsissions; }

private:
    constexpr friend bool operator==(FileStatus const& a, FileStatus const& b) {
        return a.type() == b.type() && a.permissions() == b.permissions();
    }

    FileType m_type;
    Perms m_permsissions;
};
}
