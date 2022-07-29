#pragma once

#include <liim/container/path.h>
#include <liim/string.h>

#include "error.h"

namespace PortManager {
class Config {
public:
    static Result<Config, Error> create();

    Config(const Config&) = delete;
    Config(Config&&) = default;
    ~Config();

    PathView iros_source_directory() const { return m_iros_source_directory; }
    PathView iros_build_directory() const { return m_iros_build_directory; }
    PathView iros_sysroot() const { return m_iros_sysroot; }
    PathView port_build_directory() const { return m_port_build_directory; }
    PathView port_json_directory() const { return m_port_json_directory; }
    StringView install_prefix() const { return m_install_prefix.view(); }

    Path base_directory_for_port(StringView name, StringView version) const;
    Path source_directory_for_port(StringView name, StringView version) const;
    Path build_directory_for_port(StringView name, StringView version) const;
    Path port_json_for_port(const PortHandle& handle) const;

    StringView target_architecture() const { return m_target_architecture.view(); }
    StringView target_host() const { return m_target_host.view(); }

private:
    Config(Path iros_source_directory, Path iros_build_directory, Path iros_sysroot, Path port_build_directory, Path port_json_directory,
           String install_prefix, String target_architecture, String target_host);

    Path m_iros_source_directory;
    Path m_iros_build_directory;
    Path m_iros_sysroot;
    Path m_port_build_directory;
    Path m_port_json_directory;
    String m_install_prefix;
    String m_target_architecture;
    String m_target_host;
};
}
