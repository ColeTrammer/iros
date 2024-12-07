#pragma once

#include "di/container/string/string.h"
#include "di/container/string/string_view.h"
#include "di/util/clone.h"

namespace dius::net {
class UnixAddress {
public:
    constexpr explicit UnixAddress(di::TransparentString path) : m_path(di::move(path)) {}

    constexpr auto path() const -> di::TransparentStringView { return m_path; }

    constexpr auto clone() const -> UnixAddress { return UnixAddress(di::clone(m_path)); }

private:
    di::TransparentString m_path;
};
}
