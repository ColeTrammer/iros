#pragma once

#include <ext/json.h>
#include <ext/path.h>
#include <liim/forward.h>

#include "error.h"

namespace PortManager {
class JsonReader {
public:
    static Result<JsonReader, Error> try_create(Ext::Path path);

    JsonReader(const JsonReader&) = delete;
    JsonReader(JsonReader&&) = default;
    ~JsonReader();

    const Ext::Json::Object& json() const { return m_json; }

    template<typename T>
    Result<const T&, JsonLookupError> lookup(const Ext::Json::Object& object, const String& key) const {
        return object.get_as<T>(key).unwrap_or_else([&] {
            return JsonLookupError(m_path, key, String(Ext::Json::type_to_name<T>));
        });
    }

    template<typename... Types>
    Result<Variant<const Types&...>, JsonLookupError> lookup_one_of(const Ext::Json::Object& object, const String& key) const {
        return object.get_one_of<Types...>(key).unwrap_or_else([&] {
            auto type_strings = NewVector<StringView> {};
            (type_strings.push_back(Ext::Json::type_to_name<Types>), ...);
            auto type = "["s;
            bool first = true;
            for (auto& s : type_strings) {
                if (!first) {
                    type += " or ";
                }
                type += String(s);
                first = false;
            }
            type += "]";
            return JsonLookupError(m_path, key, move(type));
        });
    }

private:
    JsonReader(Ext::Json::Object object, Ext::Path path);

    Ext::Json::Object m_json;
    Ext::Path m_path;
};
}
