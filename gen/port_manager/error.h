#pragma once

#include <ext/error.h>
#include <liim/container/path.h>
#include <liim/error/string_domain.h>
#include <liim/error/system_domain.h>
#include <liim/format.h>
#include <liim/variant.h>

#include "forward.h"

namespace PortManager {
class JsonLookupError {
public:
    JsonLookupError(Path path, String key, String type) : m_path(move(path)), m_key(move(key)), m_type(type) {}

    String to_message() const { return format("Missing required {} key `{}' in json file `{}'", m_type, m_key, m_path); }

private:
    Path m_path;
    String m_key;
    String m_type;
};

class BuildStepNotFound {
public:
    BuildStepNotFound(Path port_json, String port_name, String build_step)
        : m_port_json(move(port_json)), m_port_name(move(port_name)), m_build_step(move(build_step)) {}

    String to_message() const {
        return format("Build step `{}' not defined for port `{}' with json file `{}'", m_build_step, m_port_name, m_port_json);
    }

private:
    Path m_port_json;
    String m_port_name;
    String m_build_step;
};

using Error = Variant<StringError, JsonLookupError, BuildStepNotFound>;
}
