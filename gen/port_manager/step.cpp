#include <liim/format.h>
#include <liim/function.h>
#include <liim/result.h>
#include <liim/string_view.h>
#include <liim/try.h>

#include "context.h"
#include "error.h"
#include "json_reader.h"
#include "port.h"
#include "process.h"
#include "step.h"

namespace PortManager {
Result<UniquePtr<DownloadStep>, Error> DownloadStep::try_create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& type = TRY(reader.lookup<Ext::Json::String>(object, "type"));
    if (type.view() == "git"sv) {
        return GitDownloadStep::try_create(reader, object);
    }

    return Err(Ext::StringError(format("Unknown download type: `{}'", type)));
}

Result<UniquePtr<GitDownloadStep>, Error> GitDownloadStep::try_create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& url = TRY(reader.lookup<Ext::Json::String>(object, "url"));
    return Ok(make_unique<GitDownloadStep>(move(url)));
}

GitDownloadStep::GitDownloadStep(String url) : m_url(move(url)) {}

GitDownloadStep::~GitDownloadStep() {}

Result<bool, Error> GitDownloadStep::should_skip(Context&, const Port& port) {
    return Ok(port.source_directory().exists());
}

Result<Monostate, Error> GitDownloadStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("git", "clone", "--depth=1", m_url, port.source_directory()));
}

Result<UniquePtr<PatchStep>, Error> PatchStep::try_create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& files = TRY(reader.lookup<Ext::Json::Array>(object, "files"));

    auto patch_files = NewVector<String> {};
    for (auto& file : files) {
        if (!file.is<Ext::Json::String>()) {
            return Err(Ext::StringError("encountered non-string value when parsing patch files"));
        }
        patch_files.push_back(file.as<Ext::Json::String>());
    }

    return Ok(make_unique<PatchStep>(move(patch_files)));
}

PatchStep::PatchStep(NewVector<String> patch_files) : m_patch_files(move(patch_files)) {}

PatchStep::~PatchStep() {}

Result<bool, Error> PatchStep::should_skip(Context&, const Port& port) {
    for (auto& patch_file : m_patch_files) {
        if (!patch_marker_path(port, patch_file).exists()) {
            return Ok(false);
        }
    }
    return Ok(true);
}

Result<Monostate, Error> PatchStep::act(Context& context, const Port& port) {
    return context.with_working_directory(port.source_directory(), [&]() -> Result<Monostate, Error> {
        TRY(context.run_process(Process::command("git", "init")));

        return Ext::stop_on_error(m_patch_files, [&](auto& patch_file) -> Result<Monostate, Error> {
            auto marker_path = patch_marker_path(port, patch_file);
            if (marker_path.exists()) {
                return Ok(Monostate {});
            }
            TRY(context.run_process(Process::command("git", "apply", patch_path(port, patch_file))));
            return context.run_process(Process::command("touch", patch_marker_path(port, patch_file)));
        });
    });
}

Span<const StringView> PatchStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv };
    return storage.span();
}

Ext::Path PatchStep::patch_path(const Port& port, const String& patch_name) const {
    return port.definition_directory().join_component(patch_name);
}

Ext::Path PatchStep::patch_marker_path(const Port& port, const String& patch_name) const {
    return port.source_directory().join_component(format(".did_{}", patch_name));
}

Span<const StringView> ConfigureStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv, "patch"sv };
    return storage.span();
}

Result<UniquePtr<CMakeConfigureStep>, Error> CMakeConfigureStep::try_create(const JsonReader&, const Ext::Json::Object&) {
    return Ok(make_unique<CMakeConfigureStep>());
}

CMakeConfigureStep::~CMakeConfigureStep() {}

Result<bool, Error> CMakeConfigureStep::should_skip(Context&, const Port& port) {
    return Ok(port.build_directory().exists());
}

Result<Monostate, Error> CMakeConfigureStep::act(Context& context, const Port& port) {
    auto& config = context.config();

    auto cmake_generator = "Ninja";
    auto toolchain_file = config.iros_source_directory().join_component("cmake").join_component(
        format("CMakeToolchain_{}.txt", config.target_architecture()));
    return context.run_process(Process::command("cmake", "-S", port.source_directory(), "-B", port.build_directory(), "-G", cmake_generator,
                                                format("-DCMAKE_INSTALL_PREFIX={}", config.install_prefix()),
                                                format("-DCMAKE_TOOLCHAIN_FILE={}", toolchain_file)));
}

Span<const StringView> BuildStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv, "patch"sv, "configure"sv };
    return storage.span();
}

Result<UniquePtr<CMakeBuildStep>, Error> CMakeBuildStep::try_create(const JsonReader&, const Ext::Json::Object&) {
    return Ok(make_unique<CMakeBuildStep>());
}

CMakeBuildStep::~CMakeBuildStep() {}

Result<Monostate, Error> CMakeBuildStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("cmake", "--build", port.build_directory()));
}

Span<const StringView> InstallStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv, "patch"sv, "configure"sv, "build"sv };
    return storage.span();
}

Result<UniquePtr<CMakeInstallStep>, Error> CMakeInstallStep::try_create(const JsonReader&, const Ext::Json::Object&) {
    return Ok(make_unique<CMakeInstallStep>());
}

CMakeInstallStep::~CMakeInstallStep() {}

Result<Monostate, Error> CMakeInstallStep::act(Context& context, const Port& port) {
    auto enviornment = Enviornment::current().set("DESTDIR", port.build_directory().to_string());
    return context.run_process(Process::command("cmake", "--install", port.build_directory()).with_enviornment(move(enviornment)));
}

Result<UniquePtr<CleanStep>, Error> CleanStep::try_create() {
    return Ok(make_unique<CleanStep>());
}

CleanStep::~CleanStep() {}

Result<Monostate, Error> CleanStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("rm", "-rf", port.base_directory().to_string()));
}
}
