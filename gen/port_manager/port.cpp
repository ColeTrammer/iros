#include <ext/error.h>
#include <ext/json.h>
#include <ext/path.h>
#include <liim/container/new_vector.h>
#include <liim/generator.h>
#include <liim/pointers.h>
#include <liim/try.h>
#include <liim/tuple.h>

#include "context.h"
#include "json_reader.h"
#include "port.h"
#include "step.h"

namespace PortManager {
Result<Port, Error> Port::create(Context& context, Ext::Path json_path) {
    auto reader = TRY(JsonReader::create(json_path));

    auto& name = TRY(reader.lookup<Ext::Json::String>(reader.json(), "name"));
    auto& version = TRY(reader.lookup<Ext::Json::String>(reader.json(), "version"));

    auto& download_object = TRY(reader.lookup<Ext::Json::Object>(reader.json(), "download"));
    auto patch_object = reader.lookup<Ext::Json::Object>(reader.json(), "patch");
    auto& build_system_object = TRY(reader.lookup<Ext::Json::Object>(reader.json(), "buildSystem"));
    auto& configure_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "configure"));
    auto& build_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "build"));
    auto& install_object = TRY(reader.lookup<Ext::Json::Object>(build_system_object, "install"));

    auto build_system_type = TRY(reader.lookup<Ext::Json::String>(build_system_object, "type"));

    auto generate_steps = [&]() -> Generator<Result<UniquePtr<Step>, Error>> {
        co_yield DownloadStep::create(reader, download_object);
        if (patch_object.has_value()) {
            co_yield PatchStep::create(reader, patch_object.value());
        }

        if (build_system_type == "cmake"sv) {
            co_yield CMakeConfigureStep::create(reader, configure_object);
            co_yield CMakeBuildStep::create(reader, build_object);
            co_yield CMakeInstallStep::create(reader, install_object);
        } else if (build_system_type == "autoconf"sv) {
            co_yield AutoconfConfigureStep::create(reader, configure_object);
            co_yield AutoconfBuildStep::create(reader, build_object);
            co_yield AutoconfInstallStep::create(reader, install_object);

        } else {
            co_yield Err(Ext::StringError(format("Invalid build system type `{}' in json file `{}'", build_system_type, json_path)));
        }

        co_yield CleanStep::create();
    };

    auto steps = TRY(collect<LIIM::Container::HashMap<StringView, UniquePtr<Step>>>(transform(generate_steps(), [&](auto&& step_result) {
        return move(step_result).transform([](auto&& step) {
            return Tuple { step->name(), move(step) };
        });
    })));

    auto dependencies = NewVector<PortHandle> {};
    if (reader.json().get("dependencies")) {
        TRY(assign_to(dependencies, transform(TRY(reader.lookup<Ext::Json::Array>(reader.json(), "dependencies")), [&](auto& value) {
                          return value.template get_if<String>().unwrap_or_else([&] {
                              return format("Expected dependcy item `{}' for `{}' to be a string", Ext::Json::stringify(value), name);
                          });
                      })));
    }

    auto& config = context.config();
    auto definition_directory = json_path.dirname();
    auto base_directory = config.base_directory_for_port(name.view(), version.view());
    auto source_directory = config.source_directory_for_port(name.view(), version.view());
    auto build_directory = config.build_directory_for_port(name.view(), version.view());

    return Port(move(name), move(version), move(json_path), move(definition_directory), move(base_directory), move(source_directory),
                move(build_directory), move(steps), move(dependencies));
}

Port::Port(String name, String version, Ext::Path definition_file, Ext::Path definition_directory, Ext::Path base_directory,
           Ext::Path source_directory, Ext::Path build_directory, LIIM::Container::HashMap<StringView, UniquePtr<Step>> steps,
           NewVector<PortHandle> dependencies)
    : m_name(move(name))
    , m_version(move(version))
    , m_definition_file(move(definition_file))
    , m_definition_directory(move(definition_directory))
    , m_base_directory(move(base_directory))
    , m_source_directory(move(source_directory))
    , m_build_directory(move(build_directory))
    , m_steps(move(steps))
    , m_dependencies(move(dependencies)) {}

Port::~Port() {}

Result<void, Error> Port::build(Context& context, StringView build_step) {
    // FIXME: it would be better to topologically sort the step and its
    //        dependencies rather than just assume everything is in order
    //        and dependencies don't have their own dependencies.
    auto& base_step = TRY(lookup_step(build_step));
    auto dependencies = base_step.dependencies();
    auto steps = TRY(collect<NewVector<Step*>>(transform(dependencies, [&](auto dependency) {
        return lookup_step(dependency).transform([](auto& step) {
            return &step;
        });
    })));
    steps.push_back(&base_step);

    debug_log("Building step `{}' for port `{} {}'", build_step, name(), version());

    for (auto [i, step] : enumerate(steps)) {
        auto should_skip = TRY(step->should_skip(context, *this));
        debug_log("{} step [{} / {}]: {}", should_skip ? "Skip" : "Run", i + 1, steps.size(), step->name());
        if (!should_skip) {
            TRY(step->act(context, *this));
        }
    }
    return {};
}

Result<void, Error> Port::load_dependencies(Context& context) {
    if (m_loaded_dependencies) {
        return {};
    }
    m_loaded_dependencies = true;

    auto our_handle = this->handle();
    return Ext::stop_on_error(m_dependencies, [&](auto& dependency) {
        return context.load_port_dependency(dependency, our_handle);
    });
}

Result<Step&, BuildStepNotFound> Port::lookup_step(StringView step_name) {
    return m_steps.at(step_name)
        .map([](auto& x) -> Step& {
            return *x;
        })
        .unwrap_or_else([&] {
            return BuildStepNotFound(definition_file(), name(), String { step_name });
        });
}
}
