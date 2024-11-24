#include "config.h"

namespace pm {
Config::Config(Args const&) {
    m_iros_source_directory = "/build/iros"_p;
    m_iros_build_directory = m_iros_source_directory.clone() / "build/x86_64/gcc/release/default"_pv;
    m_iros_sysroot = m_iros_source_directory.clone() / "build/x86_64/sysroot"_pv;
    m_package_build_directory = m_iros_build_directory.clone() / "packages"_pv;
    m_package_json_directory = m_iros_source_directory.clone() / "ports"_pv;

    m_install_prefix = "/usr/local"_ts;
    m_target_architecture = "x86_64"_ts;
    m_target_host = "iros"_ts;
}

auto Config::package_patch_directory(di::TransparentStringView name, di::TransparentStringView) const -> di::Path {
    return package_json_directory().to_owned() / name;
}

auto Config::package_patch_path(di::TransparentStringView name, di::TransparentStringView version,
                                di::TransparentStringView patch) const -> di::Path {
    return package_patch_directory(name, version) / patch;
}

auto Config::package_json_for_package(di::TransparentStringView name) const -> di::Path {
    return package_json_directory().to_owned() / name / "port.json"_tsv;
}

auto Config::base_directory_for_package(di::TransparentStringView name, di::TransparentStringView version) const
    -> di::Path {
    return package_build_directory().to_owned() / name / version;
}

auto Config::source_directory_for_package(di::TransparentStringView name, di::TransparentStringView version) const
    -> di::Path {
    return base_directory_for_package(name, version) / "src"_tsv;
}

auto Config::build_directory_for_package(di::TransparentStringView name, di::TransparentStringView version) const
    -> di::Path {
    return base_directory_for_package(name, version) / "build"_tsv;
}
}
