#include <di/function/monad/monad_try.h>
#include <dius/main.h>

#include "config.h"
#include "package_database.h"
#include "package_manager.h"

namespace pm {
auto main(Args& arguments) -> di::Result<> {
    auto config = Config(arguments);

    auto database = PackageDatabase();
    auto& package = TRY(database.load_package(config, arguments.package_name));

    return package.build(config);
}
}

DIUS_MAIN(pm::Args, pm)
