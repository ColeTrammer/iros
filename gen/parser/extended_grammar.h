#pragma once

#include "item_set.h"

constexpr int END_SET = -1;

struct ItemStateTransition {
    int start;
    int end;

    bool operator==(const ItemStateTransition& other) const { return this->start == other.start && this->end == other.end; }
};

struct ExtendedInfo {
    StringView name;
    ItemStateTransition transition;

    bool operator==(const ExtendedInfo& other) const { return this->name == other.name && this->transition == other.transition; }
    bool operator!=(const ExtendedInfo& other) const { return !(*this == other); }

    String stringify() const {
        String ret = "";
        char buffer[250];
        snprintf(buffer, 250, "%d|%s|%d", transition.start, String(name).string(), transition.end);
        return String(buffer);
    }
};

namespace LIIM {

template<>
struct Traits<ExtendedInfo> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const ExtendedInfo& info) {
        unsigned int v = 0;
        v += Traits<int>::hash(info.transition.start);
        v += Traits<int>::hash(info.transition.end);
        v += Traits<StringView>::hash(info.name);
        return v;
    }
};

}

class ExtendedRule {
public:
    explicit ExtendedRule(const ExtendedInfo& info, int original_number) : m_lhs(info), m_original_number(original_number) {}

    ExtendedInfo& lhs() { return m_lhs; }
    const ExtendedInfo& lhs() const { return m_lhs; }

    Vector<ExtendedInfo>& components() { return m_components; }
    const Vector<ExtendedInfo>& components() const { return m_components; }

    int original_number() const { return m_original_number; }

    String stringify(int i = -1) const {
        String ret = "";
        if (i != -1) {
            ret += String::format("[%2d] ", i);
        }

        ret += m_lhs.stringify();
        ret += " -> ";
        m_components.for_each([&](auto& info) {
            ret += info.stringify();
            ret += " ";
        });
        return ret;
    };

    bool operator==(const ExtendedRule& other) const { return this->lhs() == other.lhs() && this->components() == other.components(); }

private:
    ExtendedInfo m_lhs;
    Vector<ExtendedInfo> m_components;
    int m_original_number;
};

class ExtendedGrammar {
public:
    ExtendedGrammar(const Vector<SharedPtr<ItemSet>>& sets, const Vector<StringView>& token_types);
    ~ExtendedGrammar();

    const Vector<ExtendedRule>& rules() const { return m_rules; }

    String stringify() const {
        String ret = "";
        int i = 0;
        m_rules.for_each([&](auto& rule) {
            ret += String::format("%-40s", rule.stringify(i++).string());
            ret += " {";
            (*m_first_sets.get(rule.lhs()))->for_each_key([&](auto& st) {
                ret += " ";
                ret += st;
            });
            ret += " } {";
            (*m_follow_sets.get(rule.lhs()))->for_each_key([&](auto& st) {
                ret += " ";
                ret += st;
            });
            ret += " }\n";
        });
        return ret;
    }

    const HashMap<ExtendedInfo, SharedPtr<HashMap<StringView, bool>>>& follow_sets() const { return m_follow_sets; }
    const Vector<SharedPtr<ItemSet>>& sets() const { return m_sets; }

private:
    void compute_first_sets();
    void compute_follow_sets();

    const Vector<SharedPtr<ItemSet>>& m_sets;
    const Vector<StringView>& m_token_types;
    Vector<ExtendedRule> m_rules;
    HashMap<ExtendedInfo, SharedPtr<HashMap<StringView, bool>>> m_first_sets;
    HashMap<ExtendedInfo, SharedPtr<HashMap<StringView, bool>>> m_follow_sets;
};
