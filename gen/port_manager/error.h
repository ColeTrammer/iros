#pragma once

#include <ext/error.h>
#include <ext/path.h>
#include <liim/format.h>
#include <liim/variant.h>

#include "forward.h"

namespace PortManager {
class JsonLookupError {
public:
    JsonLookupError(Ext::Path path, String key, StringView type) : m_path(move(path)), m_key(move(key)), m_type(type) {}

    String to_message() const { return format("Missing required {} key `{}' in json file `{}'", m_type, m_key, m_path); }

private:
    Ext::Path m_path;
    String m_key;
    StringView m_type;
};

using Error = Variant<Ext::StringError, JsonLookupError>;
}
