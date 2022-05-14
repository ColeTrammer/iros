#include <liim/result.h>
#include <liim/try.h>

#include "config.h"

namespace PortManager {
Result<Config, Error> Config::try_create() {
    auto resolve_path = [&](const String& path) {
        return Ext::Path::resolve(path).unwrap_or_else([&] {
            return StringError(format("Unable to resolve path `{}'", path));
        });
    };

    // NOTE: this is source directory which built the program.
    auto default_iros_source_directory = IROS_ROOT;
    auto iros_source_directory = TRY(resolve_path(default_iros_source_directory));

    // NOTE: this is the architecture the program was built under.
    auto default_target_architecture = IROS_ARCH;
    auto target_architecture = default_target_architecture;

    auto iros_build_directory = iros_source_directory.join_component(format("build_{}", target_architecture)).join_component("iros");

    auto iros_sysroot = iros_build_directory.join_component("sysroot");
    auto port_build_directory = iros_build_directory.join_component("ports");

    // FIXME: allow the user to override these variables
    //        through command line arguments or environment
    //        variables.
    return Ok(Config(move(iros_source_directory), move(iros_build_directory), move(iros_sysroot), move(port_build_directory),
                     move(target_architecture)));
}

Config::Config(Ext::Path iros_source_directory, Ext::Path iros_build_directory, Ext::Path iros_sysroot, Ext::Path port_build_directory,
               String target_architecture)
    : m_iros_source_directory(move(iros_source_directory))
    , m_iros_build_directory(move(iros_build_directory))
    , m_iros_sysroot(move(iros_sysroot))
    , m_port_build_directory(move(port_build_directory))
    , m_target_architecture(move(target_architecture)) {}

Config::~Config() {}

Ext::Path Config::base_directory_for_port(StringView name, StringView version) const {
    return port_build_directory().join_component(format("{}", name)).join_component(format("{}", version));
}

Ext::Path Config::source_directory_for_port(StringView name, StringView version) const {
    return base_directory_for_port(name, version).join_component("src");
}

Ext::Path Config::build_directory_for_port(StringView name, StringView version) const {
    return base_directory_for_port(name, version).join_component("build");
}
}
