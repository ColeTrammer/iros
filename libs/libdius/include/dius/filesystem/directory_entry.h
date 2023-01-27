#pragma once

#include <di/prelude.h>
#include <dius/filesystem/file_status.h>

namespace dius::filesystem {

class DirectionEntry {
public:
    DirectionEntry() = default;

    DirectionEntry(DirectionEntry&&) = default;

    DirectionEntry& operator=(DirectionEntry&&) = default;

    constexpr di::Path const& path() const& { return m_path; }
    constexpr di::Path&& path() && { return di::move(m_path); }
    constexpr di::PathView path_view() const { return m_path.view(); }

    constexpr operator di::Path const&() const& { return path(); }
    constexpr operator di::Path&&() && { return di::move(*this).path(); }
    constexpr operator di::PathView() const { return path_view(); }

private:
    di::Path m_path;
};
}