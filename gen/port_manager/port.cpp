#include <ext/json.h>
#include <ext/path.h>
#include <liim/new_vector.h>
#include <liim/pointers.h>
#include <liim/try.h>
#include <liim/tuple.h>

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

    using StepConstructor = Result<UniquePtr<Step>, Error> (*)(const JsonReader&, const Ext::Json::Object&);
    using BuildSystemConstructors = Tuple<StepConstructor, StepConstructor, StepConstructor>;

    auto build_system_type = TRY(reader.lookup<Ext::Json::String>(build_system_object, "type"));
    auto [make_configure_step, make_build_step, make_install_step] = TRY([&]() -> Result<BuildSystemConstructors, Error> {
        if (build_system_type.view() == "cmake"sv) {
            return Ok(BuildSystemConstructors(CMakeConfigureStep::try_create, CMakeBuildStep::try_create, CMakeInstallStep::try_create));
        } else if (build_system_type.view() == "autoconf"sv) {
            return Ok(
                BuildSystemConstructors(AutoconfConfigureStep::try_create, AutoconfBuildStep::try_create, AutoconfInstallStep::try_create));
        }
        return Err(Ext::StringError(format("Invalid build system type `{}' in json file `{}'", build_system_type, json_path)));
    }());

    auto& configure_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "configure"));
    auto& build_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "build"));
    auto& install_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "install"));

    auto steps = HashMap<StringView, UniquePtr<Step>> {};
    auto add_step = [&](UniquePtr<Step> step) {
        auto name = step->name();
        steps.put(name, move(step));
    };

    add_step(TRY(DownloadStep::try_create(reader, download_object)));
    if (patch_object.is_ok()) {
        add_step(TRY(PatchStep::try_create(reader, patch_object.value())));
    }
    add_step(TRY(make_configure_step(reader, configure_object)));
    add_step(TRY(make_build_step(reader, build_object)));
    add_step(TRY(make_install_step(reader, install_object)));
    add_step(TRY(CleanStep::try_create()));

    auto definition_directory = json_path.dirname();
    auto base_directory = config.base_directory_for_port(name.view(), version.view());
    auto source_directory = config.source_directory_for_port(name.view(), version.view());
    auto build_directory = config.build_directory_for_port(name.view(), version.view());

    return Ok(Port(move(name), move(version), move(json_path), move(definition_directory), move(base_directory), move(source_directory),
                   move(build_directory), move(steps)));
}

Port::Port(String name, String version, Ext::Path definition_file, Ext::Path definition_directory, Ext::Path base_directory,
           Ext::Path source_directory, Ext::Path build_directory, HashMap<StringView, UniquePtr<Step>> steps)
    : m_name(move(name))
    , m_version(move(version))
    , m_definition_file(move(definition_file))
    , m_definition_directory(move(definition_directory))
    , m_base_directory(move(base_directory))
    , m_source_directory(move(source_directory))
    , m_build_directory(move(build_directory))
    , m_steps(move(steps)) {}

Port::~Port() {}

Result<Monostate, Error> Port::build(Context& context, StringView build_step) {
    // FIXME: it would be better to topologically sort the step and its
    //        dependencies rather than just assume everything is in order
    //        and dependencies don't have their won dependencies.
    auto steps = NewVector<Step*> {};
    auto& base_step = TRY(lookup_step(build_step));
    auto dependencies = base_step.dependencies();
    for (auto dependency : dependencies) {
        steps.push_back(&TRY(lookup_step(dependency)));
    }
    steps.push_back(&base_step);

    debug_log("Building step `{}' for port `{} {}'", build_step, name(), version());

    for (auto [i, step] : enumerate(steps)) {
        auto should_skip = TRY(step->should_skip(context, *this));
        debug_log("{} step [{} / {}]: {}", should_skip ? "Skip" : "Run", i + 1, steps.size(), step->name());
        if (!should_skip) {
            TRY(step->act(context, *this));
        }
    }
    return Ok(Monostate {});
}

Result<Step&, BuildStepNotFound> Port::lookup_step(StringView step_name) {
    return m_steps.get(step_name)
        .map([](auto& x) -> Step& {
            return *x;
        })
        .unwrap_or_else([&] {
            return BuildStepNotFound(definition_file(), name(), String { step_name });
        });
}
}
