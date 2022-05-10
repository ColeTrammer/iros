#include <ext/json.h>
#include <ext/path.h>
#include <liim/result.h>
#include <liim/try.h>

#include "context.h"
#include "port.h"
#include "step.h"

namespace PortManager {
Result<Monostate, String> main(String json_path) {
    auto path = TRY(Ext::Path::resolve(json_path).unwrap_or_else([&] {
        return format("Failed to lookup path: `{}'", json_path);
    }));

    auto port = TRY(Port::try_create(path));
    return port.build();
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
