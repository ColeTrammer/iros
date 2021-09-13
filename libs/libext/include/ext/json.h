#pragma once

#include <ctype.h>
#include <ext/mapped_file.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <liim/variant.h>
#include <stdlib.h>
#include <sys/mman.h>

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

        bool empty() const { return m_map.empty(); }
        int size() const { return m_map.size(); }

        bool operator==(const Object& other) const { return this->m_map == other.m_map; }
        bool operator!=(const Object& other) const { return this->m_map != other.m_map; }

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

    static inline bool is_white_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

    static inline void do_newline(String& s, int step) {
        if (step != 0) {
            s += "\n";
        }
    }

    static inline void do_indent(String& s, int indent) {
        for (int i = 0; i < indent; i++) {
            s += " ";
        }
    }

    class InputStream {
    public:
        InputStream(const StringView& view) : m_input(view) {}

        bool at_end() const { return m_index == m_input.size(); }

        void skip_white_space() {
            while (!at_end()) {
                if (!is_white_space(m_input[m_index])) {
                    return;
                }
                m_index++;
            }
        }

        bool starts_with(const StringView& view) const {
            if (m_input.size() - m_index < view.size()) {
                return false;
            }
            for (size_t i = 0; i < view.size(); i++) {
                if (m_input[m_index + i] != view[i]) {
                    return false;
                }
            }
            return true;
        }

        void skip(size_t n) { m_index += n; }
        Maybe<char> peek(size_t i = 0) const {
            if (m_index + i >= m_input.size()) {
                return {};
            }
            return m_input[m_index + i];
        }
        bool consume(const StringView& view) {
            if (!starts_with(view)) {
                return false;
            }
            skip(view.size());
            return true;
        }

        // private:
        StringView m_input;
        size_t m_index { 0 };
    };

    static inline LIIM::String stringify(const Null&, int step = 4, int indent = 0);
    static inline LIIM::String stringify(const Boolean& b, int step = 4, int indent = 0);
    static inline LIIM::String stringify(const Number& n, int step = 4, int indent = 0);
    static inline LIIM::String stringify(const String& s, int step = 4, int indent = 0);
    static inline LIIM::String stringify(const Array& a, int step = 4, int indent = 0);
    static inline LIIM::String stringify(const Object& o, int step = 4, int indent = 0);
    static inline LIIM::String stringify(const Value& value, int step = 4, int indent = 0);

    static inline Maybe<Null> parse_null(InputStream& stream);
    static inline Maybe<Boolean> parse_boolean(InputStream& stream);
    static inline Maybe<Number> parse_number(InputStream& stream);
    static inline Maybe<String> parse_string(InputStream& stream);
    static inline Maybe<Array> parse_array(InputStream& stream);
    static inline Maybe<Object> parse_object(InputStream& stream);
    static inline Maybe<Value> parse_value(InputStream& stream);
    static inline Maybe<Object> parse(const StringView& view);

    static inline LIIM::String stringify(const Null&, int, int) { return "null"; }
    static inline LIIM::String stringify(const Boolean& b, int, int) { return b ? "true" : "false"; }
    static inline LIIM::String stringify(const Number& n, int, int) { return LIIM::String::format("%f", n); }
    static inline LIIM::String stringify(const String& s, int, int) {
        // FIXME: Escape characters if needed
        return LIIM::String::format("\"%s\"", s.string());
    }
    static inline LIIM::String stringify(const Array& a, int step, int indent) {
        LIIM::String result = "[";
        for (int i = 0; i < a.size(); i++) {
            if (i != 0) {
                result += ",";
            }
            do_newline(result, step);
            do_indent(result, indent + step);
            result += stringify(a[i], step, indent + step);
        }

        if (!a.empty()) {
            do_newline(result, step);
            do_indent(result, indent);
        }
        result += "]";
        return result;
    }
    static inline LIIM::String stringify(const Object& o, int step, int indent) {
        LIIM::String result = "{";
        bool first = true;
        o.for_each([&](auto& key, auto& value) {
            if (!first) {
                result += ",";
            }
            first = false;

            do_newline(result, step);
            do_indent(result, indent + step);

            result += stringify(key);
            result += ":";
            if (step != 0) {
                result += " ";
            }
            result += stringify(value, step, indent + step);
        });

        if (!o.empty()) {
            do_newline(result, step);
            do_indent(result, indent);
        }
        result += "}";
        return result;
    }
    static inline LIIM::String stringify(const Value& value, int step, int indent) {
        return LIIM::visit(
            [&](auto&& x) {
                return stringify(x, step, indent);
            },
            value);
    }

    static inline Maybe<Null> parse_null(InputStream& stream) {
        if (!stream.starts_with("null")) {
            return {};
        }
        return Null();
    }
    static inline Maybe<Boolean> parse_boolean(InputStream& stream) {
        if (stream.starts_with("false")) {
            return false;
        } else if (stream.starts_with("true")) {
            return true;
        }
        return {};
    }
    static inline Maybe<Number> parse_number(InputStream& stream) {
        String acc;
        if (stream.consume("-")) {
            acc += "-";
        }

        if (stream.consume("0")) {
            acc += "0";
        } else {
            bool atleast_one = false;
            for (auto c = stream.peek(); c.has_value() && isdigit(c.value()); c = stream.peek()) {
                atleast_one = true;
                acc += String(c.value());
                stream.skip(1);
            }
            if (!atleast_one) {
                return {};
            }
        }

        if (stream.consume(".")) {
            acc += ".";

            bool atleast_one = false;
            for (auto c = stream.peek(); c.has_value() && isdigit(c.value()); c = stream.peek()) {
                atleast_one = true;
                acc += String(c.value());
                stream.skip(1);
            }
            if (!atleast_one) {
                return {};
            }
        }

        if (stream.consume("e") || stream.consume("E")) {
            acc += String(stream.peek(-1).value());

            if (stream.consume("-") || stream.consume("+")) {
                acc += String(stream.peek(-1).value());
            }

            bool atleast_one = false;
            for (auto c = stream.peek(); c.has_value() && isdigit(c.value()); c = stream.peek()) {
                atleast_one = true;
                acc += String(c.value());
                stream.skip(1);
            }
            if (!atleast_one) {
                return {};
            }
        }

        return strtod(acc.string(), nullptr);
    }
    static inline Maybe<String> parse_string(InputStream& stream) {
        if (!stream.consume("\"")) {
            return {};
        }

        // FIXME: handle escape sequences
        String acc;
        while (!stream.consume("\"")) {
            auto c = stream.peek();
            if (!c.has_value()) {
                return {};
            }
            acc += String(c.value());
            stream.skip(1);
        }
        return acc;
    }
    static inline Maybe<Array> parse_array(InputStream& stream) {
        if (!stream.consume("[")) {
            return {};
        }
        auto array = Array();
        bool first = true;
        for (stream.skip_white_space(); !stream.consume("]"); stream.skip_white_space()) {
            if (!first) {
                if (!stream.consume(",")) {
                    return {};
                }
                stream.skip_white_space();
            }
            first = false;

            auto value = parse_value(stream);
            if (!value.has_value()) {
                return {};
            }
            array.add(move(value.value()));
        }
        return array;
    }
    static inline Maybe<Object> parse_object(InputStream& stream) {
        if (!stream.consume("{")) {
            return {};
        }
        auto object = Object();
        bool first = true;
        for (stream.skip_white_space(); !stream.consume("}"); stream.skip_white_space()) {
            if (!first) {
                if (!stream.consume(",")) {
                    return {};
                }
                stream.skip_white_space();
            }
            first = false;

            auto key = parse_string(stream);
            if (!key.has_value()) {
                return {};
            }
            stream.skip_white_space();
            if (!stream.consume(":")) {
                return {};
            }
            stream.skip_white_space();
            auto value = parse_value(stream);
            if (!value.has_value()) {
                return {};
            }
            object.put<Value>(key.value(), move(value.value()));
        }
        return object;
    }
    static inline Maybe<Value> parse_value(InputStream& stream) {
        auto first = stream.peek();
        if (!first.has_value()) {
            return {};
        }
        switch (first.value()) {
            case '{':
                return parse_object(stream);
            case '[':
                return parse_array(stream);
            case '"':
                return parse_string(stream);
            case 'n':
                return parse_null(stream);
            case 'f':
            case 't':
                return parse_boolean(stream);
            default:
                return parse_number(stream);
        }
    }
    static inline Maybe<Object> parse(const StringView& view) {
        auto stream = InputStream(view);
        stream.skip_white_space();
        auto result = parse_object(stream);
        stream.skip_white_space();
        if (!stream.at_end()) {
            return {};
        }
        return result;
    }

    static inline Maybe<Object> parse_file(const String& path) {
        auto file = MappedFile::try_create(path, PROT_READ, MAP_SHARED);
        if (!file) {
            return {};
        }

        auto view = StringView { (char*) file->data(), file->size() };
        return parse(view);
    }

    static inline bool write_file(const Object& object, const String& path) {
        FILE* f = fopen(path.string(), "w+");
        if (!f) {
            return false;
        }

        bool ret = true;
        auto s = stringify(object);
        if (fputs(s.string(), f) == EOF) {
            ret = false;
        }

        if (fclose(f) == EOF) {
            ret = false;
        }
        return ret;
    }
}
}
