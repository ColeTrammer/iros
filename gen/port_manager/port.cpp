#include <ext/json.h>
#include <ext/path.h>
#include <liim/pointers.h>
#include <liim/try.h>

#include "context.h"
#include "json_reader.h"
#include "port.h"
#include "step.h"

namespace PortManager {
Result<Port, Error> Port::try_create(Ext::Path path) {
    auto reader = TRY(JsonReader::try_create(path));

    auto& name = TRY(reader.lookup<Ext::Json::String>(reader.json(), "name"));
    auto& version = TRY(reader.lookup<Ext::Json::String>(reader.json(), "version"));

    auto& download_object = TRY(reader.lookup<Ext::Json::Object>(reader.json(), "download"));

    auto steps = Vector<UniquePtr<Step>> {};
    steps.add(TRY(DownloadStep::try_create(reader, download_object)));

    return Ok(Port(move(name), move(version), move(steps)));
}

Port::Port(String name, String version, Vector<UniquePtr<Step>> steps)
    : m_name(move(name)), m_version(move(version)), m_steps(move(steps)) {}

Port::~Port() {}

Result<Monostate, Error> Port::build() {
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
