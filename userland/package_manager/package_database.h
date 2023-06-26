#pragma once

#include <di/container/string/string_view.h>
#include <di/container/tree/tree_set.h>
#include <di/vocab/error/result.h>

#include "config.h"
#include "package.h"

namespace pm {
class PackageDatabase {
public:
    auto load_package(Config const& config, di::TransparentStringView name) -> di::Result<Package&>;

private:
    di::TreeSet<Package> m_packages;
};
}
