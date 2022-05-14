#include <ext/json.h>
#include <ext/path.h>
#include <liim/pointers.h>
#include <liim/try.h>

#include "context.h"
#include "json_reader.h"
#include "port.h"
#include "step.h"

namespace PortManager {
Result<Port, Error> Port::try_create(const Config& config, Ext::Path json_path) {
    auto reader = TRY(JsonReader::try_create(json_path));

    auto& name = TRY(reader.lookup<Ext::Json::String>(reader.json(), "name"));
    auto& version = TRY(reader.lookup<Ext::Json::String>(reader.json(), "version"));

    auto& download_object = TRY(reader.lookup<Ext::Json::Object>(reader.json(), "download"));
    auto patch_object = reader.lookup<Ext::Json::Object>(reader.json(), "patch");
    auto& build_system_object = TRY(reader.lookup<Ext::Json::Object>(reader.json(), "buildSystem"));

    auto build_system_type = TRY(reader.lookup<Ext::Json::String>(build_system_object, "type"));
    if (build_system_type.view() != "cmake"sv) {
        return Err(StringError(format("Invalid build system type `{}' in json file `{}'", build_system_type, json_path)));
    }

    auto& configure_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "configure"));
    auto& build_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "build"));
    auto& install_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "install"));

    auto steps = Vector<UniquePtr<Step>> {};
    steps.add(TRY(DownloadStep::try_create(reader, download_object)));
    if (patch_object.is_ok()) {
        steps.add(TRY(PatchStep::try_create(reader, patch_object.value())));
    }
    steps.add(TRY(CMakeConfigureStep::try_create(reader, configure_object)));
    steps.add(TRY(CMakeBuildStep::try_create(reader, build_object)));
    steps.add(TRY(CMakeInstallStep::try_create(reader, install_object)));

    auto definition_directory = json_path.dirname();
    auto base_directory = config.base_directory_for_port(name.view(), version.view());
    auto source_directory = config.source_directory_for_port(name.view(), version.view());
    auto build_directory = config.build_directory_for_port(name.view(), version.view());

    return Ok(Port(move(name), move(version), move(definition_directory), move(base_directory), move(source_directory),
                   move(build_directory), move(steps)));
}

Port::Port(String name, String version, Ext::Path definition_directory, Ext::Path base_directory, Ext::Path source_directory,
           Ext::Path build_directory, Vector<UniquePtr<Step>> steps)
    : m_name(move(name))
    , m_version(move(version))
    , m_definition_directory(move(definition_directory))
    , m_base_directory(move(base_directory))
    , m_source_directory(move(source_directory))
    , m_build_directory(move(build_directory))
    , m_steps(move(steps)) {}

Port::~Port() {}

Result<Monostate, Error> Port::build(Context& context) {
    debug_log("Building port: {} {}", name(), version());

    auto steps = m_steps.size();
    for (int i = 0; i < steps; i++) {
        auto& step = *m_steps[i];
        debug_log("Run step [{} / {}]: {}", i + 1, steps, step.name());
        TRY(step.act(context, *this));
    }
    return Ok(Monostate {});
}
}
