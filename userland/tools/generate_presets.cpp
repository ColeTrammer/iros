#include <di/cli/prelude.h>
#include <di/container/view/prelude.h>
#include <di/reflect/prelude.h>
#include <di/serialization/json_deserializer.h>
#include <di/serialization/json_serializer.h>
#include <dius/main.h>
#include <dius/print.h>
#include <dius/sync_file.h>
#include <dius/system/process.h>

namespace generate_presets {
struct Args {
    di::PathView output { "CMakePresets.json"_pv };
    bool prettier { false };

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("generate_presets"_sv, "Generate CMake presets file"_sv)
            .flag<&Args::output>('o', "output"_tsv, "Output file path"_sv)
            .flag<&Args::prettier>('p', "prettier"_tsv, "Run Prettier on output"_sv);
    }
};

struct CMakeVersion {
    int major;
    int minor;
    int patch;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeVersion>) {
        return di::make_fields(di::field<"major", &CMakeVersion::major>, di::field<"minor", &CMakeVersion::minor>,
                               di::field<"patch", &CMakeVersion::patch>);
    }
};

struct CMakeConfigurePreset {
    di::String name;
    di::Optional<di::String> display_name;
    di::Optional<di::String> description;
    di::Optional<di::String> binary_directory;
    di::Optional<di::String> install_directory;
    di::Optional<bool> hidden;
    di::Optional<di::String> generator;
    di::Optional<di::String> toolchain_file;
    di::Optional<di::TreeMap<di::String, di::String>> cache_variables;
    di::Optional<di::Vector<di::String>> inherits;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeConfigurePreset>) {
        return di::make_fields(di::field<"name", &CMakeConfigurePreset::name>,
                               di::field<"displayName", &CMakeConfigurePreset::display_name>,
                               di::field<"description", &CMakeConfigurePreset::description>,
                               di::field<"binaryDir", &CMakeConfigurePreset::binary_directory>,
                               di::field<"installDir", &CMakeConfigurePreset::install_directory>,
                               di::field<"hidden", &CMakeConfigurePreset::hidden>,
                               di::field<"generator", &CMakeConfigurePreset::generator>,
                               di::field<"toolchainFile", &CMakeConfigurePreset::toolchain_file>,
                               di::field<"cacheVariables", &CMakeConfigurePreset::cache_variables>,
                               di::field<"inherits", &CMakeConfigurePreset::inherits>);
    }
};

struct CMakeBuildPreset {
    di::String name;
    di::Optional<di::String> display_name;
    di::Optional<di::String> description;
    di::Optional<bool> hidden;
    di::Optional<di::String> configure_preset;
    di::Optional<di::Vector<di::String>> targets;
    di::Optional<di::Vector<di::String>> inherits;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeBuildPreset>) {
        return di::make_fields(
            di::field<"name", &CMakeBuildPreset::name>, di::field<"displayName", &CMakeBuildPreset::display_name>,
            di::field<"description", &CMakeBuildPreset::description>, di::field<"hidden", &CMakeBuildPreset::hidden>,
            di::field<"configurePreset", &CMakeBuildPreset::configure_preset>,
            di::field<"targets", &CMakeBuildPreset::targets>, di::field<"inherits", &CMakeBuildPreset::inherits>);
    }
};

struct CMakeTestOutput {
    di::Optional<bool> output_on_failure;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeTestOutput>) {
        return di::make_fields(di::field<"outputOnFailure", &CMakeTestOutput::output_on_failure>);
    }
};

enum class CMakeTestExecutionNoTestsAction {
    Default,
    Error,
    Ignore,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeTestExecutionNoTestsAction>) {
    return di::make_enumerators(di::enumerator<"default", CMakeTestExecutionNoTestsAction::Default>,
                                di::enumerator<"error", CMakeTestExecutionNoTestsAction::Error>,
                                di::enumerator<"ignore", CMakeTestExecutionNoTestsAction::Ignore>);
}

enum class CMakeTestExecutionRepeatMode {
    UntilFail,
    UntilPass,
    AfterTimeout,
};

constexpr auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeTestExecutionRepeatMode>) {
    return di::make_enumerators(di::enumerator<"until-fail", CMakeTestExecutionRepeatMode::UntilFail>,
                                di::enumerator<"until-pass", CMakeTestExecutionRepeatMode::UntilPass>,
                                di::enumerator<"after-timeout", CMakeTestExecutionRepeatMode::AfterTimeout>);
}

struct CMakeTestExecutionRepeat {
    int count;
    CMakeTestExecutionRepeatMode mode;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeTestExecutionRepeat>) {
        return di::make_fields(di::field<"count", &CMakeTestExecutionRepeat::count>,
                               di::field<"mode", &CMakeTestExecutionRepeat::mode>);
    }
};

struct CMakeTestExecution {
    di::Optional<int> timeout;
    di::Optional<bool> stop_on_failure;
    di::Optional<CMakeTestExecutionRepeat> repeat;
    di::Optional<CMakeTestExecutionNoTestsAction> no_tests_action;
    di::Optional<int> jobs;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeTestExecution>) {
        return di::make_fields(di::field<"timeout", &CMakeTestExecution::timeout>,
                               di::field<"stopOnFailure", &CMakeTestExecution::stop_on_failure>,
                               di::field<"repeat", &CMakeTestExecution::repeat>,
                               di::field<"noTestsAction", &CMakeTestExecution::no_tests_action>,
                               di::field<"jobs", &CMakeTestExecution::jobs>);
    }
};

struct CMakeTestPreset {
    di::String name;
    di::Optional<di::String> display_name;
    di::Optional<di::String> description;
    di::Optional<bool> hidden;
    di::Optional<di::String> configure_preset;
    di::Optional<CMakeTestOutput> output;
    di::Optional<CMakeTestExecution> execution;
    di::Optional<di::Vector<di::String>> inherits;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakeTestPreset>) {
        return di::make_fields(
            di::field<"name", &CMakeTestPreset::name>, di::field<"displayName", &CMakeTestPreset::display_name>,
            di::field<"description", &CMakeTestPreset::description>, di::field<"hidden", &CMakeTestPreset::hidden>,
            di::field<"configurePreset", &CMakeTestPreset::configure_preset>,
            di::field<"output", &CMakeTestPreset::output>, di::field<"execution", &CMakeTestPreset::execution>,
            di::field<"inherits", &CMakeTestPreset::inherits>);
    }
};

struct CMakePresets {
    int version;
    CMakeVersion cmake_minimum_required;
    di::Vector<CMakeConfigurePreset> configure_presets;
    di::Vector<CMakeBuildPreset> build_presets;
    di::Vector<CMakeTestPreset> test_presets;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<CMakePresets>) {
        return di::make_fields(di::field<"version", &CMakePresets::version>,
                               di::field<"cmakeMinimumRequired", &CMakePresets::cmake_minimum_required>,
                               di::field<"configurePresets", &CMakePresets::configure_presets>,
                               di::field<"buildPresets", &CMakePresets::build_presets>,
                               di::field<"testPresets", &CMakePresets::test_presets>);
    }
};

static di::Tuple<di::Vector<CMakeConfigurePreset>, di::Vector<di::String>> make_configure_presets() {
    auto presets = di::Vector<CMakeConfigurePreset> {};

    auto base_presets = *di::from_json_string<di::Vector<CMakeConfigurePreset>>(R"([
        {
            "name": "base",
            "hidden": true,
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
                "CMAKE_INSTALL_MESSAGE": "NEVER",
                "IROS_WarningFlags": "-Wall -Wextra -Wpedantic",
                "CMAKE_VERIFY_INTERFACE_HEADER_SETS": "ON"
            }
        },
        {
            "name": "gcc_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_DiagnosticFlags": "-fdiagnostics-color=always -ffold-simple-inlines"
            }
        },
        {
            "name": "clang_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_DiagnosticFlags": "-fcolor-diagnostics -fconstexpr-steps=10000000",
                "IROS_WarningFlags": "-Wall -Wextra -Wpedantic",
                "CMAKE_CXX_COMPILER": "clang++-16"
            }
        },
        {
            "name": "release_base",
            "hidden": true,
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            },
            "inherits": ["base"]
        },
        {
            "name": "debug_base",
            "hidden": true,
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            },
            "inherits": ["base"]
        },
        {
            "name": "iros_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildIris": "ON",
                "IROS_BuildUserspace": "ON",
                "IROS_BuildTests": "ON",
                "IROS_BuildCcpp": "ON",
                "IROS_BuildTools": "ON",
                "IROS_UseDiusRuntime": "ON",
                "IROS_ExtraFlags": "-D__iros__=1 -U__linux__",
                "CMAKE_CXX_COMPILER_WORKS": "ON"
            }
        },
        {
            "name": "iris_debug_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_IrisFlags": "-Og -g"
            }
        },
        {
            "name": "iros_x86_64_base",
            "hidden": true,
            "inherits": ["iros_base"]
        },
        {
            "name": "dius_runtime_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_UseDiusRuntime": "ON",
                "IROS_BuildCcpp": "ON",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "tsan_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildCcpp": "OFF",
                "IROS_SanitizerFlags": "-fsanitize=thread -DDI_SANITIZER",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "asan_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildCcpp": "OFF",
                "IROS_SanitizerFlags": "-fsanitize=address -DDI_SANITIZER",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "ubsan_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildCcpp": "OFF",
                "IROS_SanitizerFlags": "-fsanitize=undefined -DDI_SANITIZER",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "msan_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildCcpp": "OFF",
                "IROS_SanitizerFlags": "-fsanitize=memory -DDI_SANITIZER",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "ubasan_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildCcpp": "OFF",
                "IROS_SanitizerFlags": "-fsanitize=address,undefined -DDI_SANITIZER",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "coverage_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildCcpp": "OFF",
                "IROS_ExtraFlags": "--coverage",
                "IROS_BuildTests": "ON"
            }
        },
        {
            "name": "tools_base",
            "hidden": true,
            "cacheVariables": {
                "IROS_BuildTests": "OFF",
                "IROS_BuildUserspace": "OFF",
                "IROS_BuildTools": "ON",
                "IROS_BuildCcpp": "OFF"
            }
        }
    ])"_sv);
    presets.append_container(base_presets | di::as_rvalue);

    auto preset_names = di::Vector<di::String> {};

    auto iros_compilers = di::Array { "gcc"_sv, "clang"_sv };
    auto iros_compiler_names = di::TreeMap<di::String, di::String> {};
    iros_compiler_names["gcc"_sv] = "GCC"_sv.to_owned();
    iros_compiler_names["clang"_sv] = "Clang"_sv.to_owned();

    auto iros_build_types = di::Array { "release"_sv, "debug"_sv };
    auto iros_build_types_names = di::TreeMap<di::String, di::String> {};
    iros_build_types_names["release"_sv] = "Release"_sv.to_owned();
    iros_build_types_names["debug"_sv] = "Debug"_sv.to_owned();

    auto iros_build_features = di::Array { "default"_sv, "iris_debug"_sv };
    auto iros_build_feature_names = di::TreeMap<di::String, di::String> {};
    iros_build_feature_names["default"_sv] = "Default"_sv.to_owned();
    iros_build_feature_names["iris_debug"_sv] = "Iris Debug"_sv.to_owned();

    auto iros_architectures = di::Array { "x86_64"_sv };
    auto iros_architectures_names = di::TreeMap<di::String, di::String> {};
    iros_architectures_names["x86_64"_sv] = "Iros x86_64"_sv.to_owned();

    auto unity_build = di::Array { true, false };
    for (auto [compiler, build_type, arch, build_feature, unity] : di::cartesian_product(
             iros_compilers, iros_build_types, iros_architectures, iros_build_features, unity_build)) {
        if (build_type == "debug"_sv && build_feature == "iris_debug"_sv) {
            continue;
        }

        auto name = *di::present("{}_iros_{}_{}_{}"_sv, compiler, arch, build_type, build_feature);
        auto display_name =
            *di::present("{} {} {} {}"_sv, iros_compiler_names[compiler], iros_architectures_names[arch],
                         iros_build_types_names[build_type], iros_build_feature_names[build_feature]);
        auto description = *di::present("Iros {} {} {} build using {}"_sv, arch, build_type, build_feature, compiler);
        auto build_directory_suffix = *di::present("{}/{}/{}/{}"_sv, arch, compiler, build_type, build_feature);
        if (!unity) {
            display_name.append(" (Non Unity)"_sv);
            name.append("_non_unity"_sv);
            build_directory_suffix.append("/non_unity"_sv);
        }
        auto build_directory = *di::present("${{sourceDir}}/build/{}"_sv, build_directory_suffix);
        auto install_directory = *di::present("${{sourceDir}}/build/{}/sysroot/usr"_sv, arch);
        auto toolchain_file = *di::present("${{sourceDir}}/meta/cmake/CMakeToolchain_Iros_{}_{}.txt"_sv,
                                           compiler == "gcc"_sv ? "Gcc"_sv : "Clang"_sv, arch);

        preset_names.push_back(di::clone(name));

        auto inherits = di::Vector<di::String> {};
        inherits.push_back(*di::present("{}_base"_sv, compiler));
        inherits.push_back(*di::present("{}_base"_sv, build_type));
        inherits.push_back(*di::present("iros_{}_base"_sv, arch));
        if (build_feature != "default"_sv) {
            inherits.push_back(*di::present("{}_base"_sv, build_feature));
        }

        auto cache_variables = di::TreeMap<di::String, di::String> {};
        cache_variables.insert_or_assign("CMAKE_UNITY_BUILD"_sv, unity ? "ON"_sv.to_owned() : "OFF"_sv.to_owned());
        cache_variables.insert_or_assign("CMAKE_EXPORT_COMPILE_COMMANDS"_sv,
                                         unity ? "OFF"_sv.to_owned() : "ON"_sv.to_owned());
        cache_variables.insert_or_assign(
            "CMAKE_CROSSCOMPILING_EMULATOR"_sv,
            *di::present("${{sourceDir}}/meta/run-command-on-iris.sh;{}"_sv, build_directory_suffix));
        if (unity) {
            cache_variables.insert_or_assign("IROS_NonUnityBuildPreset"_sv, *di::present("{}_non_unity"_sv, name));
        }

        auto preset = CMakeConfigurePreset {};
        preset.name = di::move(name);
        preset.display_name = di::move(display_name);
        preset.description = di::move(description);
        preset.binary_directory = di::move(build_directory);
        preset.install_directory = di::move(install_directory);
        preset.inherits = di::move(inherits);
        preset.toolchain_file = di::move(toolchain_file);
        preset.cache_variables = di::move(cache_variables);

        presets.push_back(di::move(preset));
    }

    auto compilers = di::Array { "clang"_sv, "gcc"_sv };
    auto compiler_names = di::TreeMap<di::String, di::String> {};
    compiler_names["clang"_sv] = "Clang"_sv.to_owned();
    compiler_names["gcc"_sv] = "GCC"_sv.to_owned();

    auto build_types = di::Array { "release"_sv, "debug"_sv };
    auto build_types_names = di::TreeMap<di::String, di::String> {};
    build_types_names["release"_sv] = "Release"_sv.to_owned();
    build_types_names["debug"_sv] = "Debug"_sv.to_owned();

    auto build_features = di::Array { "dius_runtime"_sv, "ubasan"_sv, "default"_sv, "coverage"_sv, "tools"_sv,
                                      "asan"_sv,         "ubsan"_sv,  "tsan"_sv,    "msan"_sv };

    auto build_feature_names = di::TreeMap<di::String, di::String> {};
    build_feature_names["dius_runtime"_sv] = "Dius Runtime"_sv.to_owned();
    build_feature_names["ubasan"_sv] = "Address and Undefined Behavior Sanitizer"_sv.to_owned();
    build_feature_names["default"_sv] = "Default"_sv.to_owned();
    build_feature_names["coverage"_sv] = "Coverage"_sv.to_owned();
    build_feature_names["tools"_sv] = "Tools"_sv.to_owned();
    build_feature_names["asan"_sv] = "Address Sanitizer"_sv.to_owned();
    build_feature_names["ubsan"_sv] = "Undefined Behavior Sanitizer"_sv.to_owned();
    build_feature_names["tsan"_sv] = "Thread Sanitizer"_sv.to_owned();
    build_feature_names["msan"_sv] = "Memory Sanitizer"_sv.to_owned();

    for (auto [compiler, build_type, build_feature, unity] :
         di::cartesian_product(compilers, build_types, build_features, unity_build)) {
        if (compiler == "gcc"_sv && build_feature == "msan"_sv) {
            continue;
        }

        auto name = *di::present("{}_{}_{}"_sv, compiler, build_type, build_feature);
        auto display_name = *di::present("{} {} {}"_sv, compiler_names[compiler], build_types_names[build_type],
                                         build_feature_names[build_feature]);
        auto description = *di::present("{} {} build using {}"_sv, build_type, build_feature, compiler);
        auto build_directory =
            *di::present("${{sourceDir}}/build/host/{}/{}/{}"_sv, compiler, build_type, build_feature);
        if (!unity) {
            display_name.append(" (Non Unity)"_sv);
            name.append("_non_unity"_sv);
            build_directory.append("/non_unity"_sv);
        }
        auto install_directory = *di::present("{}/install"_sv, build_directory);

        preset_names.push_back(di::clone(name));

        auto inherits = di::Vector<di::String> {};
        inherits.push_back(*di::present("{}_base"_sv, compiler));
        inherits.push_back(*di::present("{}_base"_sv, build_type));
        if (build_feature != "default"_sv) {
            inherits.push_back(*di::present("{}_base"_sv, build_feature));
        }

        auto cache_variables = di::TreeMap<di::String, di::String> {};
        cache_variables.insert_or_assign("CMAKE_UNITY_BUILD"_sv, unity ? "ON"_sv.to_owned() : "OFF"_sv.to_owned());
        cache_variables.insert_or_assign("CMAKE_EXPORT_COMPILE_COMMANDS"_sv,
                                         unity ? "OFF"_sv.to_owned() : "ON"_sv.to_owned());
        if (unity) {
            cache_variables.insert_or_assign("IROS_NonUnityBuildPreset"_sv, *di::present("{}_non_unity"_sv, name));
        }

        auto preset = CMakeConfigurePreset {};
        preset.name = di::move(name);
        preset.display_name = di::move(display_name);
        preset.description = di::move(description);
        preset.binary_directory = di::move(build_directory);
        preset.install_directory = di::move(install_directory);
        preset.inherits = di::move(inherits);
        preset.cache_variables = di::move(cache_variables);

        presets.push_back(di::move(preset));
    }

    di::stable_partition(presets, [](auto const& preset) {
        return !preset.name.contains("non_unity"_sv);
    });

    return { di::move(presets), di::move(preset_names) };
}

di::Vector<CMakeBuildPreset> make_build_presets(di::Span<di::String const> preset_names) {
    auto presets = di::Vector<CMakeBuildPreset> {};

    auto base_presets = *di::from_json_string<di::Vector<CMakeBuildPreset>>(R"([
        {
            "name": "build_base",
            "hidden": true
        },
        {
            "name": "ci_base",
            "hidden": true,
            "targets": ["all", "all_verify_interface_header_sets"]
        },
        {
            "name": "docs_base",
            "hidden": true,
            "targets": ["docs"]
        }
    ])"_sv);
    presets.append_container(base_presets | di::as_rvalue);

    auto build_types = di::Array { "default"_sv, "ci"_sv, "docs"_sv };
    auto display_names = di::Array { "Build All"_sv, "Build for CI"_sv, "Build Docs"_sv };
    auto descriptions =
        di::Array { "Build all targets"_sv, "Build all targets and verify headers"_sv, "Build documentation"_sv };
    for (auto const& configure_name : preset_names) {
        for (auto [display_name, description, type] : di::zip(display_names, descriptions, build_types)) {
            auto name = [&] {
                if (type != "default"_sv) {
                    return *di::present("{}_{}"_sv, configure_name, type);
                }
                return di::clone(configure_name);
            }();

            auto inherits = di::Vector<di::String> {};
            inherits.push_back("build_base"_sv.to_owned());
            if (type != "default"_sv) {
                inherits.push_back(*di::present("{}_base"_sv, type));
            }

            auto preset = CMakeBuildPreset {};
            preset.name = di::move(name);
            preset.display_name = display_name.to_owned();
            preset.description = description.to_owned();
            preset.configure_preset = di::clone(configure_name);
            preset.inherits = di::move(inherits);
            presets.push_back(di::move(preset));
        }
    }
    return presets;
}

di::Vector<CMakeTestPreset> make_test_presets(di::Span<di::String const> preset_names) {
    auto presets = di::Vector<CMakeTestPreset> {};

    auto base_presets = *di::from_json_string<di::Vector<CMakeTestPreset>>(R"([
        {
            "name": "test_base",
            "hidden": true,
            "output": {
                "outputOnFailure": true
            },
            "execution": {
                "noTestsAction": "error",
                "stopOnFailure": false,
                "timeout": 30
            }
        },
        {
            "name": "iros_base",
            "hidden": true,
            "execution": {
                "jobs": 1,
                "repeat": {
                    "mode": "until-pass",
                    "count": 2
                }
            }
        }
    ])"_sv);
    presets.append_container(base_presets | di::as_rvalue);

    auto test_types = di::Array { "default"_sv };
    auto display_names = di::Array { "Test All"_sv };
    auto descriptions = di::Array { "Test all targets"_sv };
    for (auto const& configure_name : preset_names) {
        for (auto [display_name, description, type] : di::zip(display_names, descriptions, test_types)) {
            auto name = [&] {
                if (type != "default"_sv) {
                    return *di::present("{}_{}"_sv, configure_name, type);
                }
                return di::clone(configure_name);
            }();

            auto inherits = di::Vector<di::String> {};
            inherits.push_back("test_base"_sv.to_owned());
            if (configure_name.contains("iros"_sv)) {
                inherits.push_back("iros_base"_sv.to_owned());
            }
            if (type != "default"_sv) {
                inherits.push_back(*di::present("{}_base"_sv, type));
            }

            auto preset = CMakeTestPreset {};
            preset.name = di::move(name);
            preset.display_name = display_name.to_owned();
            preset.description = description.to_owned();
            preset.configure_preset = di::clone(configure_name);
            preset.inherits = di::move(inherits);
            presets.push_back(di::move(preset));
        }
    }
    return presets;
}

di::Result<void> main(Args& args) {
    auto [configure_presets, configure_preset_names] = make_configure_presets();
    auto build_presets = make_build_presets(configure_preset_names.span());
    auto test_presets = make_test_presets(configure_preset_names.span());
    auto presets =
        CMakePresets { 4, { 3, 25, 2 }, di::move(configure_presets), di::move(build_presets), di::move(test_presets) };

    auto file = TRY(dius::open_sync(args.output, dius::OpenMode::WriteClobber));
    TRY(di::serialize_json(file, presets, di::JsonSerializerConfig().pretty()));

    if (args.prettier) {
        auto command = di::Vector<di::TransparentString> {};
        command.push_back("prettier"_tsv.to_owned());
        command.push_back("--write"_tsv.to_owned());
        command.push_back(args.output.data().to_owned());
        TRY(dius::system::Process(di::move(command)).spawn_and_wait());
    }

    return {};
}
}

DIUS_MAIN(generate_presets::Args, generate_presets)
