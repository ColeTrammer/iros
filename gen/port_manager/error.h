#pragma once

#include <ext/path.h>
#include <liim/format.h>
#include <liim/variant.h>

#include "forward.h"

namespace PortManager {
class StringError {
public:
    StringError(String message) : m_message(move(message)) {}

    String to_message() const { return m_message; }

private:
    String m_message;
};

class JsonLookupError {
public:
    JsonLookupError(Ext::Path path, String key, StringView type) : m_path(move(path)), m_key(move(key)), m_type(type) {}

    String to_message() const { return format("Missing required {} key `{}' in json file `{}'", m_type, m_path, m_path); }

private:
    Ext::Path m_path;
    String m_key;
    StringView m_type;
};

using Error = Variant<StringError, JsonLookupError>;
}

namespace LIIM::Format {
template<>
struct Formatter<PortManager::Error> : Formatter<StringView> {
    void format(const PortManager::Error& value, FormatContext& context) {
        auto message = value.visit([](auto&& error) {
            return error.to_message();
        });
        return format_string_view(message.view(), context);
    }
};
}
