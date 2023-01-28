#pragma once

#include "extended_grammar.h"

class FinalRule {
public:
    FinalRule(const ExtendedRule& rule, const HashMap<StringView, bool> follow_set)
        : m_lhs(rule.lhs().name)
        , m_number(rule.lhs().transition.start)
        , m_original_number(rule.original_number())
        , m_follow_set(follow_set) {
        rule.components().for_each([&](auto& part) {
            m_components.add(part.name);
            m_number = part.transition.end;
        });
    }
    ~FinalRule() {}

    int number() const { return m_number; }
    int original_number() const { return m_original_number; }
    HashMap<StringView, bool>& follow_set() { return m_follow_set; }
    const HashMap<StringView, bool>& follow_set() const { return m_follow_set; }
    const StringView& lhs() const { return m_lhs; }
    const Vector<StringView>& components() const { return m_components; }

    String stringify() const {
        String ret = String::format("%-2d ", m_number);
        ret += String(m_lhs);
        ret += " ->";
        m_components.for_each([&](auto& s) {
            ret += " ";
            ret += String(s);
        });
        ret += " {";
        m_follow_set.for_each_key([&](auto& s) {
            ret += " ";
            ret += String(s);
        });
        ret += " }\n";
        return ret;
    }

    bool operator==(const FinalRule& other) const { return this->number() == other.number() && this->lhs() == other.lhs(); }

private:
    StringView m_lhs;
    Vector<StringView> m_components;
    int m_number;
    int m_original_number;
    HashMap<StringView, bool> m_follow_set;
};

struct Action {
    enum class Type { Accept, Shift, Jump, Reduce };
    Type type;
    int number;

    bool operator==(const Action& other) const { return this->type == other.type && this->number == other.number; }
    bool operator!=(const Action& other) const { return !(*this == other); }

    String stringify() const {
        switch (type) {
            case Type::Accept:
                return "Accect";
            case Type::Shift:
                return String::format("s%d", number);
            case Type::Jump:
                return String::format("%d", number);
            case Type::Reduce:
                return String::format("r%d", number);
        }

        return "?????";
    }
};

namespace LIIM {

template<>
struct Traits<Action> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const Action& action) {
        unsigned int v = (unsigned int) action.type;
        v += Traits<int>::hash(action.number);
        return v;
    }
};

}

class StateTable {
public:
    StateTable(const ExtendedGrammar& grammar, const Vector<StringView>& identifiers, const Vector<StringView>& token_types);
    ~StateTable();

    const ExtendedGrammar& grammar() const { return m_grammar; }
    const HashMap<int, Vector<FinalRule>> rules() const { return m_rules; }
    const Vector<HashMap<StringView, Action>>& table() const { return m_table; }

    String stringify();

private:
    const ExtendedGrammar& m_grammar;
    HashMap<int, Vector<FinalRule>> m_rules;
    Vector<HashMap<StringView, Action>> m_table;
    const Vector<StringView>& m_identifiers;
};
