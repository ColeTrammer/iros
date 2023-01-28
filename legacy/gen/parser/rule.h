#pragma once

#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/traits.h>
#include <liim/vector.h>

class Rule {
public:
    Rule() : m_name("null") {}
    Rule(StringView name, const Vector<StringView>& components, int position = 0)
        : m_name(name), m_components(components), m_position(position) {}
    Rule(const Rule& other)
        : m_name(other.name()), m_components(other.components()), m_number(other.number()), m_position(other.position()) {}
    ~Rule() {}

    StringView& name() { return m_name; }
    const StringView& name() const { return m_name; }

    int number() const { return m_number; }
    void set_number(int n) { m_number = n; }

    Vector<StringView>& components() { return m_components; }
    const Vector<StringView>& components() const { return m_components; }

    String stringify() const {
        String ret = "";
        char buf[50];
        snprintf(buf, 50, "[%-3d] ", number());
        ret += buf;
        ret += String(m_name);
        ret += " -> ";

        int i = 0;
        m_components.for_each([&](const StringView& part) {
            if (i++ == position()) {
                ret += ". ";
            }
            ret += String(part);
            ret += " ";
        });

        if (i == position()) {
            ret += ". ";
        }

        return ret;
    }

    bool operator==(const Rule& other) const {
        return this->name() == other.name() && this->components() == other.components() && this->position() == other.position();
    }
    bool operator!=(const Rule& other) const { return !(*this == other); }

    int position() const { return m_position; }
    void increment_position() { m_position++; }

private:
    StringView m_name;
    Vector<StringView> m_components;
    int m_number { 0 };
    int m_position { 0 };
};

namespace LIIM {

template<>
struct Traits<Rule> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const Rule& rule) { return static_cast<unsigned int>(rule.number()); }
};

}
