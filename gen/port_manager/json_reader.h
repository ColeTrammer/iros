#pragma once

#include <ext/json.h>
#include <ext/path.h>
#include <liim/forward.h>

#include "error.h"

namespace PortManager {
class JsonReader {
public:
    static Result<JsonReader, StringError> try_create(Ext::Path path);

    JsonReader(const JsonReader&) = delete;
    JsonReader(JsonReader&&) = default;
    ~JsonReader();

    const Ext::Json::Object& json() const { return m_json; }

    template<typename T>
    Result<const T&, JsonLookupError> lookup(const Ext::Json::Object& object, const String& key) const {
        return object.get_as<T>(key).unwrap_or_else([&] {
            return JsonLookupError(m_path, key, Ext::Json::type_to_name<T>);
        });
    }

private:
    JsonReader(Ext::Json::Object object, Ext::Path path);

    Ext::Json::Object m_json;
    Ext::Path m_path;
};
}
