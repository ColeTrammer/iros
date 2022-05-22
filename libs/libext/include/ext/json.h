#pragma once

#include <ctype.h>
#include <ext/mapped_file.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <liim/variant.h>
#include <stdlib.h>
#include <sys/mman.h>

namespace Ext::Json {

class Value;

using Null = LIIM::Monostate;
using Boolean = bool;
using Array = LIIM::Vector<Value>;
using Number = double;
using String = LIIM::String;

class Object {
public:
    template<typename T, typename... Args>
    void put(const String& name, Args&&... args) {
        auto raw_value = T(forward<Args>(args)...);
        auto value = make_unique<Value>(move(raw_value));
        m_map.put(name, move(value));
    }

    Option<Value&> get(const String& name) {
        return m_map.get(name).map([](auto& x) -> Value& {
            return *x;
        });
    }
    Option<const Value&> get(const String& name) const {
        return m_map.get(name).map([](auto& x) -> const Value& {
            return *x;
        });
    }

    template<typename T>
    Option<T&> get_as(const String& name) {
        return m_map.get(name).and_then([](auto& x) {
            return x->template get_if<T>();
        });
    }
    template<typename T>
    Option<const T&> get_as(const String& name) const {
        return m_map.get(name).and_then([](auto& x) {
            return x->template get_if<T>();
        });
    }

    template<typename... Types>
    Option<Variant<Types&...>> get_one_of(const String& name) {
        return m_map.get(name).and_then([](auto& x) {
            return x->template get_subvariant<Types...>();
        });
    }
    template<typename... Types>
    Option<Variant<const Types&...>> get_one_of(const String& name) const {
        return m_map.get(name).and_then([](auto& x) {
            return x->template get_subvariant<Types...>();
        });
    }

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

template<typename T>
constexpr StringView type_to_name = [] {
    using LIIM::IsSame;
    if constexpr (IsSame<T, Null>::value) {
        return "Null"sv;
    } else if constexpr (IsSame<T, Boolean>::value) {
        return "Boolean"sv;
    } else if constexpr (IsSame<T, Array>::value) {
        return "Array"sv;
    } else if constexpr (IsSame<T, Number>::value) {
        return "Number"sv;
    } else if constexpr (IsSame<T, Object>::value) {
        return "Object"sv;
    } else {
        return "Invalid JSON Type"sv;
    }
}();

using ValueImpl = LIIM::Variant<Null, Boolean, Object, Array, Number, String>;
class Value : public ValueImpl {
    using ValueImpl::Variant;
};

template<typename T>
T Object::get_or(const String& name, T alt) const {
    auto result = get_as<T>(name);
    if (!result) {
        return move(alt);
    }
    return *result;
}

static inline bool is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

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
    Option<char> peek(size_t i = 0) const {
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

static inline Option<Null> parse_null(InputStream& stream);
static inline Option<Boolean> parse_boolean(InputStream& stream);
static inline Option<Number> parse_number(InputStream& stream);
static inline Option<String> parse_string(InputStream& stream);
static inline Option<Array> parse_array(InputStream& stream);
static inline Option<Object> parse_object(InputStream& stream);
static inline Option<Value> parse_value(InputStream& stream);
static inline Option<Object> parse(const StringView& view);

static inline LIIM::String stringify(const Null&, int, int) {
    return "null";
}
static inline LIIM::String stringify(const Boolean& b, int, int) {
    return b ? "true" : "false";
}
static inline LIIM::String stringify(const Number& n, int, int) {
    return LIIM::String::format("%f", n);
}
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
    return value.visit([&](auto&& x) {
        return stringify(x, step, indent);
    });
}

static inline Option<Null> parse_null(InputStream& stream) {
    if (!stream.starts_with("null")) {
        return {};
    }
    stream.consume("null");
    return Null();
}
static inline Option<Boolean> parse_boolean(InputStream& stream) {
    if (stream.starts_with("false")) {
        stream.consume("false");
        return false;
    } else if (stream.starts_with("true")) {
        stream.consume("true");
        return true;
    }
    return {};
}
static inline Option<Number> parse_number(InputStream& stream) {
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
static inline Option<String> parse_string(InputStream& stream) {
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
static inline Option<Array> parse_array(InputStream& stream) {
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
static inline Option<Object> parse_object(InputStream& stream) {
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
static inline Option<Value> parse_value(InputStream& stream) {
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
static inline Option<Object> parse(const StringView& view) {
    auto stream = InputStream(view);
    stream.skip_white_space();
    auto result = parse_object(stream);
    stream.skip_white_space();
    if (!stream.at_end()) {
        return {};
    }
    return result;
}

static inline Option<Object> parse_file(const String& path) {
    auto file = try_map_file(path, PROT_READ, MAP_SHARED);
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
