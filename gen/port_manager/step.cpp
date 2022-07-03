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
Result<UniquePtr<DownloadStep>, Error> DownloadStep::create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& type = TRY(reader.lookup<Ext::Json::String>(object, "type"));
    if (type.view() == "git"sv) {
        return GitDownloadStep::create(reader, object);
    } else if (type.view() == "tarball"sv) {
        return TarDownloadStep::create(reader, object);
    }

    return Err(Ext::StringError(format("Unknown download type: `{}'", type)));
}

Result<UniquePtr<GitDownloadStep>, Error> GitDownloadStep::create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& url = TRY(reader.lookup<Ext::Json::String>(object, "url"));
    return make_unique<GitDownloadStep>(move(url));
}

GitDownloadStep::GitDownloadStep(String url) : m_url(move(url)) {}

GitDownloadStep::~GitDownloadStep() {}

Result<bool, Error> GitDownloadStep::should_skip(Context&, const Port& port) {
    return port.source_directory().exists();
}

Result<void, Error> GitDownloadStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("git", "clone", "--depth=1", m_url, port.source_directory()));
}

Result<UniquePtr<Step>, Error> TarDownloadStep::create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& url = TRY(reader.lookup<Ext::Json::String>(object, "url"));
    auto kind = TRY(kind_from_string(TRY(reader.lookup<Ext::Json::String>(object, "kind"))));
    auto& signature_url = TRY(reader.lookup<Ext::Json::String>(object, "signature"));
    auto& source_directory_in_tarball = TRY(reader.lookup<Ext::Json::String>(object, "sourceDirectory"));
    return make_unique<TarDownloadStep>(move(url), kind, move(signature_url), move(source_directory_in_tarball));
}

TarDownloadStep::TarDownloadStep(String url, Kind kind, String signature_url, String source_directory_in_tarball)
    : m_url(move(url)), m_kind(kind), m_signature_url(signature_url), m_source_directory_in_tarball(move(source_directory_in_tarball)) {}

TarDownloadStep::~TarDownloadStep() {}

Result<bool, Error> TarDownloadStep::should_skip(Context&, const Port& port) {
    return download_destination(port).exists() && signature_download_destination(port).exists() && port.source_directory().exists();
}

Result<void, Error> TarDownloadStep::act(Context& context, const Port& port) {
    auto base_directory = port.base_directory();
    TRY(context.run_process(Process::command("mkdir", "-p", base_directory)));
    switch (m_kind) {
        case Kind::Gz: {
            auto download_path = download_destination(port);
            if (!download_path.exists()) {
                TRY(context.run_process(Process::command("curl", m_url, "--output", download_path)));
            }

            auto signature_path = signature_download_destination(port);
            if (!signature_path.exists()) {
                TRY(context.run_process(Process::command("curl", m_signature_url, "--output", signature_path)));
                // FIXME: verify the downloaded signature against the tarball
            }

            auto target_source_directory = port.source_directory();
            if (!target_source_directory.exists()) {
                TRY(context.run_process(Process::command("tar", "-xzf", download_path, "--directory", base_directory)));
                TRY(context.run_process(
                    Process::command("mv", base_directory.join_component(m_source_directory_in_tarball), target_source_directory)));
            }
            break;
        }
        default:
            assert(false);
    }
    return {};
}

Ext::Path TarDownloadStep::download_destination(const Port& port) const {
    return port.base_directory().join_component(format("download.tar.{}", kind_to_string(m_kind)));
}

Ext::Path TarDownloadStep::signature_download_destination(const Port& port) const {
    return port.base_directory().join_component(format("download.tar.{}.sig", kind_to_string(m_kind)));
}

String TarDownloadStep::kind_to_string(Kind kind) {
    switch (kind) {
        case Kind::Gz:
            return "gz";
        default:
            assert(false);
    }
}

auto TarDownloadStep::kind_from_string(const String& string) -> Result<Kind, Ext::StringError> {
    if (string == "gz") {
        return Kind::Gz;
    }
    return Err(Ext::StringError(format("Unknown tarball download kind: `{}'", string)));
}

Result<UniquePtr<PatchStep>, Error> PatchStep::create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto& files = TRY(reader.lookup<Ext::Json::Array>(object, "files"));

    auto patch_files = NewVector<String> {};
    for (auto& file : files) {
        if (!file.is<Ext::Json::String>()) {
            return Err(Ext::StringError("encountered non-string value when parsing patch files"));
        }
        patch_files.push_back(file.as<Ext::Json::String>());
    }

    return make_unique<PatchStep>(move(patch_files));
}

PatchStep::PatchStep(NewVector<String> patch_files) : m_patch_files(move(patch_files)) {}

PatchStep::~PatchStep() {}

Result<bool, Error> PatchStep::should_skip(Context&, const Port& port) {
    for (auto& patch_file : m_patch_files) {
        if (!patch_marker_path(port, patch_file).exists()) {
            return false;
        }
    }
    return true;
}

Result<void, Error> PatchStep::act(Context& context, const Port& port) {
    return context.with_working_directory(port.source_directory(), [&]() -> Result<void, Error> {
        TRY(context.run_process(Process::command("git", "init")));

        return Ext::stop_on_error(m_patch_files, [&](auto& patch_file) -> Result<void, Error> {
            auto marker_path = patch_marker_path(port, patch_file);
            if (marker_path.exists()) {
                return {};
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

Result<UniquePtr<Step>, Error> CMakeConfigureStep::create(const JsonReader&, const Ext::Json::Object&) {
    return make_unique<CMakeConfigureStep>();
}

CMakeConfigureStep::~CMakeConfigureStep() {}

Result<bool, Error> CMakeConfigureStep::should_skip(Context&, const Port& port) {
    return port.build_directory().exists();
}

Result<void, Error> CMakeConfigureStep::act(Context& context, const Port& port) {
    auto& config = context.config();

    auto cmake_generator = "Ninja";
    auto toolchain_file = config.iros_source_directory().join_component("cmake").join_component(
        format("CMakeToolchain_{}.txt", config.target_architecture()));
    return context.run_process(Process::command("cmake", "-S", port.source_directory(), "-B", port.build_directory(), "-G", cmake_generator,
                                                format("-DCMAKE_INSTALL_PREFIX={}", config.install_prefix()),
                                                format("-DCMAKE_TOOLCHAIN_FILE={}", toolchain_file)));
}

Result<Enviornment, Error> AutoconfConfigureStep::parse_enviornment(const JsonReader& reader, const Ext::Json::Object& object) {
    if (!object.get("env")) {
        return Enviornment::current();
    }
    auto& enviornment = TRY(reader.lookup<Ext::Json::Object>(object, "env"));
    return Enviornment::from_json(reader, enviornment);
}

auto AutoconfConfigureStep::parse_settings(const JsonReader& reader, const Ext::Json::Object& object) -> Result<Settings, Error> {
    auto settings = Settings {};
    if (object.get("settings")) {
        auto& user_settings = TRY(reader.lookup<Ext::Json::Object>(object, "settings"));
        auto keys = NewVector<String> {};
        user_settings.for_each([&](auto& key, auto&) {
            keys.push_back(key);
        });
        for (auto& key : keys) {
            settings.insert_or_assign(
                key, TRY(reader.lookup_one_of<Ext::Json::Boolean, Ext::Json::String>(user_settings, key)).visit([](auto&& x) -> Setting {
                    return x;
                }));
        }
    }
    return settings;
}

Result<UniquePtr<Step>, Error> AutoconfConfigureStep::create(const JsonReader& reader, const Ext::Json::Object& object) {
    auto enviornment = TRY(parse_enviornment(reader, object));
    auto settings = TRY(parse_settings(reader, object));
    return make_unique<AutoconfConfigureStep>(move(enviornment), move(settings));
}

AutoconfConfigureStep::AutoconfConfigureStep(Enviornment enviornment, Settings settings)
    : m_enviornment(move(enviornment)), m_settings(move(settings)) {}

AutoconfConfigureStep::~AutoconfConfigureStep() {}

Result<bool, Error> AutoconfConfigureStep::should_skip(Context&, const Port& port) {
    return port.build_directory().exists();
}

Result<void, Error> AutoconfConfigureStep::act(Context& context, const Port& port) {
    auto& config = context.config();
    auto host = config.target_host();
    auto install_prefix = config.install_prefix();
    auto configure_script = port.source_directory().join_component("configure");
    auto build_directory = port.build_directory();

    auto arguments = NewVector<String> {};
    arguments.push_back(configure_script.to_string());
    auto settings = m_settings.clone();
    settings.try_emplace("--host", move(host));
    settings.try_emplace("--prefix", move(install_prefix));
    for (auto& [name, setting] : settings) {
        if (auto as_bool = setting.template get_if<bool>()) {
            if (*as_bool) {
                arguments.push_back(name);
            }
        } else if (auto as_string = setting.template get_if<String>()) {
            arguments.push_back(format("{}={}", name, *as_string));
        }
    }

    TRY(context.run_process(Process::command("mkdir", "-p", build_directory)));
    auto result = context.with_working_directory(build_directory, [&] {
        return context.run_process(Process::from_arguments(move(arguments)).with_enviornment(m_enviornment.clone()));
    });
    if (result.is_error()) {
        TRY(context.run_process(Process::command("rm", "-rf", build_directory)));
    }
    return result;
};

Span<const StringView> BuildStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv, "patch"sv, "configure"sv };
    return storage.span();
}

Result<UniquePtr<Step>, Error> CMakeBuildStep::create(const JsonReader&, const Ext::Json::Object&) {
    return make_unique<CMakeBuildStep>();
}

CMakeBuildStep::~CMakeBuildStep() {}

Result<void, Error> CMakeBuildStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("cmake", "--build", port.build_directory()));
}

Result<UniquePtr<Step>, Error> AutoconfBuildStep::create(const JsonReader&, const Ext::Json::Object&) {
    return make_unique<AutoconfBuildStep>();
}

AutoconfBuildStep::~AutoconfBuildStep() {}

Result<void, Error> AutoconfBuildStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("make", "-C", port.build_directory()));
}

Span<const StringView> InstallStep::dependencies() const {
    static constexpr FixedArray storage = { "download"sv, "patch"sv, "configure"sv, "build"sv };
    return storage.span();
}

Result<UniquePtr<Step>, Error> CMakeInstallStep::create(const JsonReader&, const Ext::Json::Object&) {
    return make_unique<CMakeInstallStep>();
}

CMakeInstallStep::~CMakeInstallStep() {}

Result<void, Error> CMakeInstallStep::act(Context& context, const Port& port) {
    auto enviornment = Enviornment::current().set("DESTDIR", context.config().iros_sysroot().to_string());
    return context.run_process(Process::command("cmake", "--install", port.build_directory()).with_enviornment(move(enviornment)));
}

Result<UniquePtr<Step>, Error> AutoconfInstallStep::create(const JsonReader&, const Ext::Json::Object&) {
    return make_unique<AutoconfInstallStep>();
}

AutoconfInstallStep::~AutoconfInstallStep() {}

Result<void, Error> AutoconfInstallStep::act(Context& context, const Port& port) {
    auto destination_argument = format("DESTDIR={}", context.config().iros_sysroot());
    return context.run_process(Process::command("make", "-C", port.build_directory(), "install", destination_argument));
}

Result<UniquePtr<CleanStep>, Error> CleanStep::create() {
    return make_unique<CleanStep>();
}

CleanStep::~CleanStep() {}

Result<void, Error> CleanStep::act(Context& context, const Port& port) {
    return context.run_process(Process::command("rm", "-rf", port.base_directory().to_string()));
}
}
