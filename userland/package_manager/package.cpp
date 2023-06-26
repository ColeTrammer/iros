#include <di/container/string/string_view.h>
#include <di/function/tag_invoke.h>
#include <di/platform/custom.h>
#include <di/reflect/prelude.h>
#include <di/serialization/json_deserializer.h>
#include <di/serialization/json_value.h>
#include <dius/print.h>
#include <dius/sync_file.h>
#include <dius/system/process.h>

#include "package.h"

namespace pm {
enum class PackageJsonDownloadType {
    Git,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<PackageJsonDownloadType>) {
    using enum PackageJsonDownloadType;
    return di::make_enumerators(di::enumerator<"git", Git>);
}

struct PackageJsonDownload {
    PackageJsonDownloadType type;
    di::String url;
    di::Optional<di::String> tag;
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<PackageJsonDownload>) {
    return di::make_fields(di::field<"type", &PackageJsonDownload::type>, di::field<"url", &PackageJsonDownload::url>,
                           di::field<"tag", &PackageJsonDownload::tag>);
}

struct PackageJsonPatchFormat {
    di::Vector<di::String> files;
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<PackageJsonPatchFormat>) {
    return di::make_fields(di::field<"files", &PackageJsonPatchFormat::files>);
}

struct PackageJsonFormat {
    di::String name;
    di::String version;
    PackageJsonDownload download;
    di::Optional<PackageJsonPatchFormat> patch;
    di::json::Object build_system;
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<PackageJsonFormat>) {
    return di::make_fields(
        di::field<"name", &PackageJsonFormat::name>, di::field<"version", &PackageJsonFormat::version>,
        di::field<"download", &PackageJsonFormat::download>, di::field<"patch", &PackageJsonFormat::patch>,
        di::field<"buildSystem", &PackageJsonFormat::build_system>);
}

// FIXME: This is an unfortunate workaround for the fact that the JSON types are UTF-8 strings, while we use
// unencoded strings since we have to use them as file paths. It would be nice if we did the conversion in the path
// code instead, and used UTF-8 strings everywhere else.
static auto to_transparent_string(di::StringView string) -> di::TransparentString {
    auto result = di::TransparentString();
    for (auto c : string) {
        result.push_back(char(c));
    }
    return result;
}

auto Package::load(dius::SyncFile& file) -> di::Result<Package> {
    auto package = Package();

    auto deserializer = di::JsonDeserializer(di::move(file));
    auto info = TRY(di::deserialize<PackageJsonFormat>(deserializer));

    package.m_name = to_transparent_string(info.name);
    package.m_version = to_transparent_string(info.version);

    auto steps = di::Vector<Step>();
    steps.emplace_back(
        StepKind::Download,
        di::make_function<di::Result<>(Config const&, Package&)>(
            [download = di::move(info.download)](Config const& config, Package& package) -> di::Result<> {
                dius::println("Downloading package {} {} from {}..."_sv, package.name(), package.version(),
                              download.url);

                switch (download.type) {
                    case PackageJsonDownloadType::Git: {
                        auto command = di::Vector<di::TransparentString> {};
                        command.push_back("git"_ts);
                        command.push_back("clone"_ts);
                        command.push_back("--depth=1"_ts);
                        if (download.tag) {
                            command.push_back("--branch"_ts);
                            command.push_back(to_transparent_string(download.tag.value()));
                        }
                        command.push_back(to_transparent_string(download.url));
                        command.push_back(config.source_directory_for_package(package.name(), package.version())
                                              .take_underlying_string());
                        TRY(dius::system::Process(di::move(command)).spawn_and_wait());
                        break;
                    }
                    default:
                        di::unreachable();
                }
                return {};
            }));
    if (info.patch) {
        steps.emplace_back(
            StepKind::Patch,
            di::make_function<di::Result<>(Config const&, Package&)>(
                [patch = di::move(info.patch.value())](Config const& config, Package& package) -> di::Result<> {
                    dius::println("Patching package {} {}..."_sv, package.name(), package.version());

                    for (auto const& file : patch.files) {
                        auto transparent_file = to_transparent_string(file);
                        auto patch_path =
                            config.package_patch_path(package.name(), package.version(), transparent_file);

                        auto command = di::Vector<di::TransparentString> {};
                        command.push_back("patch"_ts);
                        command.push_back("-d"_ts);
                        command.push_back(config.source_directory_for_package(package.name(), package.version())
                                              .take_underlying_string());
                        command.push_back("-ui"_ts);
                        command.push_back(di::move(patch_path).take_underlying_string());
                        TRY(dius::system::Process(di::move(command)).spawn_and_wait());
                    }
                    return {};
                }));
    }
    steps.emplace_back(StepKind::Configure, di::make_function<di::Result<>(Config const&, Package&)>(
                                                [](Config const& config, Package& package) -> di::Result<> {
                                                    dius::println("Configuring package {} {}..."_sv, package.name(),
                                                                  package.version());

                                                    // For now, just install the Iros libraries and headers. This is
                                                    // needed before configuring, since the package could use it for
                                                    // feature detection.
                                                    auto command = di::Vector<di::TransparentString> {};
                                                    command.push_back("ninja"_ts);
                                                    command.push_back("install"_ts);
                                                    command.push_back("-C"_ts);
                                                    command.push_back(config.iros_build_directory().data().to_owned());
                                                    TRY(dius::system::Process(di::move(command)).spawn_and_wait());
                                                    return {};
                                                }));
    steps.emplace_back(
        StepKind::Build, di::make_function<di::Result<>(Config const&, Package&)>([](Config const& config,
                                                                                     Package& package) -> di::Result<> {
            dius::println("Building package {} {}..."_sv, package.name(), package.version());

            // For now, assume a simple makefile.
            auto command = di::Vector<di::TransparentString> {};

            command.push_back("make"_ts);
            command.push_back("-C"_ts);
            command.push_back(
                config.source_directory_for_package(package.name(), package.version()).take_underlying_string());

            // This shouldn't be hardcoded.
            auto cc = "CC="_ts;
            cc.append("x86_64-iros-gcc"_tsv);
            command.push_back(di::move(cc));

            auto ld = "LD="_ts;
            ld.append("x86_64-iros-gcc"_tsv);
            command.push_back(di::move(ld));

            auto cflags = "CFLAGS="_ts;
            cflags.append("-static -O3"_ts);
            command.push_back(di::move(cflags));

            auto ldflags = "LDFLAGS="_ts;
            ldflags.append("-static"_ts);
            command.push_back(di::move(ldflags));

            auto destdir = "DESTDIR="_ts;
            destdir.append(config.iros_sysroot().data());
            command.push_back(di::move(destdir));

            auto prefix = "PREFIX="_ts;
            prefix.append(config.install_prefix());
            command.push_back(di::move(prefix));

            TRY(dius::system::Process(di::move(command)).spawn_and_wait());

            return {};
        }));
    steps.emplace_back(
        StepKind::Install,
        di::make_function<di::Result<>(Config const&, Package&)>(
            [](Config const& config, Package& package) -> di::Result<> {
                dius::println("Installing package {} {}..."_sv, package.name(), package.version());

                // For now, assume a simple makefile.
                auto command = di::Vector<di::TransparentString> {};
                command.push_back("make"_ts);
                command.push_back("-C"_ts);
                command.push_back(
                    config.source_directory_for_package(package.name(), package.version()).take_underlying_string());
                command.push_back("install"_ts);

                // This shouldn't be hardcoded.
                auto cc = "CC="_ts;
                cc.append("x86_64-iros-gcc"_tsv);
                command.push_back(di::move(cc));

                auto ld = "LD="_ts;
                ld.append("x86_64-iros-gcc"_tsv);
                command.push_back(di::move(ld));

                auto cflags = "CFLAGS="_ts;
                cflags.append("-static -O3"_ts);
                command.push_back(di::move(cflags));

                auto ldflags = "LDFLAGS="_ts;
                ldflags.append("-static"_ts);
                command.push_back(di::move(ldflags));

                auto destdir = "DESTDIR="_ts;
                destdir.append(config.iros_sysroot().data());
                command.push_back(di::move(destdir));

                auto prefix = "PREFIX="_ts;
                prefix.append(config.install_prefix());
                command.push_back(di::move(prefix));

                TRY(dius::system::Process(di::move(command)).spawn_and_wait());

                return {};
            }));

    package.m_steps = di::move(steps);
    return package;
}

auto Package::build(Config const& config) -> di::Result<> {
    dius::println("Building package {} {}..."_sv, m_name, m_version);

    auto step_count = m_steps.size();
    for (auto [i, step] : di::enumerate(m_steps)) {
        dius::eprintln("[{}/{}] Running step {}..."_sv, i + 1, step_count, step.kind());
        TRY(step.run(config, *this));
    }
    return {};
}
}
