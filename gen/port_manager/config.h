#pragma once

#include <ext/path.h>
#include <liim/string.h>

#include "error.h"

namespace PortManager {
class Config {
public:
    static Result<Config, Error> create();

    Config(const Config&) = delete;
    Config(Config&&) = default;
    ~Config();

    const Ext::Path& iros_source_directory() const { return m_iros_source_directory; }
    const Ext::Path& iros_build_directory() const { return m_iros_build_directory; }
    const Ext::Path& iros_sysroot() const { return m_iros_sysroot; }
    const Ext::Path& port_build_directory() const { return m_port_build_directory; }
    const Ext::Path& port_json_directory() const { return m_port_json_directory; }
    const String& install_prefix() const { return m_install_prefix; }

    Ext::Path base_directory_for_port(StringView name, StringView version) const;
    Ext::Path source_directory_for_port(StringView name, StringView version) const;
    Ext::Path build_directory_for_port(StringView name, StringView version) const;
    Ext::Path port_json_for_port(const PortHandle& handle) const;

    const String& target_architecture() const { return m_target_architecture; }
    const String& target_host() const { return m_target_host; }

private:
    Config(Ext::Path iros_source_directory, Ext::Path iros_build_directory, Ext::Path iros_sysroot, Ext::Path port_build_directory,
           Ext::Path port_json_directory, String install_prefix, String target_architecture, String target_host);

    Ext::Path m_iros_source_directory;
    Ext::Path m_iros_build_directory;
    Ext::Path m_iros_sysroot;
    Ext::Path m_port_build_directory;
    Ext::Path m_port_json_directory;
    String m_install_prefix;
    String m_target_architecture;
    String m_target_host;
};
}
