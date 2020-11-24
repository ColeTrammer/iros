#pragma once

#include <liim/hash_map.h>
#include <liim/string.h>
#include <liim/variant.h>

namespace Ext {
namespace Json {

    class Value;

    using Null = LIIM::Monostate;
    using Boolean = bool;
    using Array = LIIM::Vector<Value>;
    using Number = double;
    using String = LIIM::String;

    class Object {
    public:
        template<typename T, typename... Args>
        void put(const String& name, Args... args) {
            auto raw_value = T(forward<Args>(args)...);
            auto value = make_unique<Value>(move(raw_value));
            m_map.put(name, move(value));
        }

        Value* get(const String& name) { return const_cast<Value*>(const_cast<const Object&>(*this).get(name)); }
        const Value* get(const String& name) const {
            auto result = m_map.get(name);
            if (!result) {
                return nullptr;
            }
            return result->get();
        }

        template<typename T>
        T* get_as(const String& name) {
            return const_cast<T*>(const_cast<const Object&>(*this).get_as<T>(name));
        }
        template<typename T>
        const T* get_as(const String& name) const;

        template<typename T>
        T get_or(const String& name, T alt) const;

    private:
        LIIM::HashMap<String, UniquePtr<Value>> m_map;
    };

    using ValueImpl = LIIM::Variant<Null, Boolean, Object, Array, Number, String>;
    class Value : public ValueImpl {
        using ValueImpl::Variant;
    };

    template<typename T>
    const T* Object::get_as(const String& name) const {
        auto v = get(name);
        if (!v) {
            return nullptr;
        }
        if (!v->is<T>()) {
            return nullptr;
        }
        return &v->as<T>();
    }

    template<typename T>
    T Object::get_or(const String& name, T alt) const {
        auto* result = get_as<T>(name);
        if (!result) {
            return move(alt);
        }
        return *result;
    }
}
}
