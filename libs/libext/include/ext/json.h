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

        template<typename Callback>
        void for_each(Callback&& c) const {
            m_map.for_each_key([&](auto& key) {
                c(key, **m_map.get(key));
            });
        }

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

    LIIM::String stringify(const Null&);
    LIIM::String stringify(const Boolean& b);
    LIIM::String stringify(const Number& n);
    LIIM::String stringify(const String& s);
    LIIM::String stringify(const Array& a);
    LIIM::String stringify(const Object& o);
    LIIM::String stringify(const Value& value);

    LIIM::String stringify(const Null&) { return "null"; }
    LIIM::String stringify(const Boolean& b) { return b ? "true" : "false"; }
    LIIM::String stringify(const Number& n) { return LIIM::String::format("%f", n); }
    LIIM::String stringify(const String& s) { return LIIM::String::format("\"%s\"", s.string()); }
    LIIM::String stringify(const Array& a) {
        LIIM::String result = "[";
        for (int i = 0; i < a.size(); i++) {
            if (i != 0) {
                result += ",";
            }
            result += stringify(a[i]);
        }
        result += "]";
        return result;
    }
    LIIM::String stringify(const Object& o) {
        LIIM::String result = "{";
        bool first = true;
        o.for_each([&](auto& key, auto& value) {
            if (!first) {
                result += ",";
            }
            first = false;

            result += stringify(key);
            result += ":";
            result += stringify(value);
        });
        result += "}";
        return result;
    }
    LIIM::String stringify(const Value& value) {
        return LIIM::visit(
            [](auto&& x) {
                return stringify(x);
            },
            value);
    }
}
}
