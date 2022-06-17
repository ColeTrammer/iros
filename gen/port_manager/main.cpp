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
    String json_path { "port.json" };
    StringView build_step { "install" };
};

constexpr auto argument_parser = [] {
    using namespace Cli;
    return Parser::of<Arguments>()
        .flag(Flag::defaulted<&Arguments::json_path>().short_name('f').long_name("port-json").description("path to port json file"))
        .argument(Argument::defaulted<&Arguments::build_step>("step").description("build step to perform"));
}();

Result<void, Error> main(Arguments arguments) {
    auto context = Context(TRY(Config::try_create()));

    auto path = TRY(Ext::Path::resolve(arguments.json_path));

    auto port = TRY(Port::try_create(context.config(), move(path)));
    return port.build(context, arguments.build_step);
}
}

CLI_MAIN(PortManager::main, PortManager::argument_parser)
