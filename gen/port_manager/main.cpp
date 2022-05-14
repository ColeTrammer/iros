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
Result<Monostate, Error> main(String json_path) {
    auto context = Context(TRY(Config::try_create()));

    auto path = TRY(Ext::Path::resolve(json_path).unwrap_or_else([&] {
        return StringError(format("Failed to lookup path: `{}'", json_path));
    }));

    auto port = TRY(Port::try_create(context.config(), move(path)));
    return port.build(context);
}
}

static void print_usage_and_exit(const char* name) {
    error_log("Usage: {} <port-json-file>", name);
    exit(2);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage_and_exit(*argv);
    }

    auto result = PortManager::main(argv[1]);
    if (result.is_error()) {
        error_log("{}: {}", argv[0], result.error());
        return 1;
    }
    return 0;
}
