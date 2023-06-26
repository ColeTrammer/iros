#pragma once

#include <di/container/string/string.h>
#include <di/container/string/string_view.h>
#include <di/container/vector/vector.h>
#include <di/serialization/json_value.h>
#include <dius/sync_file.h>

#include "config.h"
#include "step.h"

namespace pm {
class Package {
public:
    static auto load(dius::SyncFile& file) -> di::Result<Package>;

    auto name() const { return m_name.view(); }
    auto version() const { return m_version.view(); }

    auto operator==(Package const& other) const { return m_name == other.m_name; }
    auto operator<=>(Package const& other) const { return m_name <=> other.m_name; }

    auto operator==(di::TransparentStringView other) const { return m_name == other; }
    auto operator<=>(di::TransparentStringView other) const { return m_name <=> other; }

    auto build(Config const& config) -> di::Result<>;

private:
    di::TransparentString m_name;
    di::TransparentString m_version;
    di::Vector<Step> m_steps;
};
}
