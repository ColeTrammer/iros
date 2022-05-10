#include <ext/json.h>
#include <ext/path.h>
#include <liim/pointers.h>
#include <liim/try.h>

#include "context.h"
#include "port.h"
#include "step.h"

namespace PortManager {
Result<Port, String> Port::try_create(const Ext::Path& path) {
    auto json = TRY(Ext::Json::parse_file(path.to_string()).unwrap_or_else([&] {
        return format("Failed to load JSON file: `{}'", path);
    }));

    auto get_string_key = [&](const Ext::Json::Object& object, const String& name) {
        return object.get_as<Ext::Json::String>(name).unwrap_or_else([&] {
            return format("Failed to find key `{}' in JSON file: `{}'", name, path);
        });
    };

    auto name = TRY(get_string_key(json, "name"));
    auto version = TRY(get_string_key(json, "version"));

    return Ok(Port(move(name), move(version), {}));
}

Port::Port(String name, String version, Vector<UniquePtr<Step>> steps)
    : m_name(move(name)), m_version(move(version)), m_steps(move(steps)) {}

Port::~Port() {}

Result<Monostate, String> Port::build() {
    debug_log("Building port: {} {}", name(), version());

    Context context;
    auto steps = m_steps.size();
    for (int i = 0; i < steps; i++) {
        auto& step = *m_steps[i];
        debug_log("Run step [{} / {}]: {}", i + 1, steps, step.name());
        TRY(step.act(context));
    }
    return Ok(Monostate {});
}
}
