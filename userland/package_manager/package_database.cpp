#include <di/serialization/json_deserializer.h>
#include <di/util/reference_wrapper.h>
#include <dius/sync_file.h>

#include "package_database.h"

namespace pm {
auto PackageDatabase::load_package(Config const& config, di::TransparentStringView name) -> di::Result<Package&> {
    auto path = config.package_json_for_package(name);
    auto file = TRY(dius::open_sync(path, dius::OpenMode::Readonly));

    auto [it, did_insert] = m_packages.insert(TRY(Package::load(file)));
    return *it;
}
}
