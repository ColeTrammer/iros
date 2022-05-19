#include <cli/cli.h>
#include <ext/json.h>
#include <ext/path.h>
#include <liim/result.h>
#include <liim/try.h>

#include "config.h"
#include "context.h"
#include "error.h"
#include "port.h"
#include "step.h"

namespace PortManager {
struct Arguments {
    String json_path;
};

constexpr auto argument_parser = [] {
    using namespace Cli;
    return Parser::of<Arguments>().argument(Argument::single<&Arguments::json_path>("port.json").description("path to port json file"));
}();

Result<Monostate, Error> main(Arguments arguments) {
    auto context = Context(TRY(Config::try_create()));

    auto path = TRY(Ext::Path::resolve(arguments.json_path).unwrap_or_else([&] {
        return Ext::StringError(format("Failed to lookup path: `{}'", arguments.json_path));
    }));

    auto port = TRY(Port::try_create(context.config(), move(path)));
    return port.build(context);
}
}

CLI_MAIN(PortManager::main, PortManager::argument_parser)
