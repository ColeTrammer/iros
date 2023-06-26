#pragma once

#include <di/container/path/path.h>
#include <di/container/path/path_view.h>
#include <di/container/string/string.h>
#include <di/container/string/string_view.h>

#include "package_manager.h"

namespace pm {
class Config {
public:
    explicit Config(Args const& arguments);

    auto iros_source_directory() const -> di::PathView { return m_iros_source_directory; }
    auto iros_build_directory() const -> di::PathView { return m_iros_build_directory; }
    auto iros_sysroot() const -> di::PathView { return m_iros_sysroot; }
    auto package_build_directory() const -> di::PathView { return m_package_build_directory; }
    auto package_json_directory() const -> di::PathView { return m_package_json_directory; }

    auto package_patch_directory(di::TransparentStringView name, di::TransparentStringView version) const -> di::Path;
    auto package_patch_path(di::TransparentStringView name, di::TransparentStringView version,
                            di::TransparentStringView patch) const -> di::Path;

    auto package_json_for_package(di::TransparentStringView name) const -> di::Path;
    auto base_directory_for_package(di::TransparentStringView name, di::TransparentStringView version) const
        -> di::Path;
    auto source_directory_for_package(di::TransparentStringView name, di::TransparentStringView version) const
        -> di::Path;
    auto build_directory_for_package(di::TransparentStringView name, di::TransparentStringView version) const
        -> di::Path;

    auto target_architecture() const -> di::TransparentStringView { return m_target_architecture; }
    auto target_host() const -> di::TransparentStringView { return m_target_host; }
    auto install_prefix() const -> di::TransparentStringView { return m_install_prefix; }

private:
    Config() = default;

    di::Path m_iros_source_directory;
    di::Path m_iros_build_directory;
    di::Path m_iros_sysroot;
    di::Path m_package_build_directory;
    di::Path m_package_json_directory;
    di::TransparentString m_install_prefix;
    di::TransparentString m_target_architecture;
    di::TransparentString m_target_host;
};
}
