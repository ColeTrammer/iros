#pragma once

#include "item_set.h"

constexpr int END_SET = -1;

struct ItemStateTransition {
    int start;
    int end;
};

struct ExtendedInfo {
    StringView name;
    ItemStateTransition transition;

    String stringify() const {
        String ret = "";
        char buffer[250];
        snprintf(buffer, 250, "%d|%s|%d", transition.start, String(name).string(), transition.end);
        return String(buffer);
    }
};

class ExtendedRule {
public:
    ExtendedRule(const ExtendedInfo& info) : m_lhs(info) {}

    ExtendedInfo& lhs() { return m_lhs; }
    const ExtendedInfo& lhs() const { return m_lhs; }

    Vector<ExtendedInfo>& components() { return m_components; }
    const Vector<ExtendedInfo>& components() const { return m_components; }

    String stringify(int i = -1) const {
        String ret = "";
        if (i != -1) {
            char buffer[50];
            snprintf(buffer, 50, "[%d] ", i);
            ret += buffer;
        }

        ret += m_lhs.stringify();
        ret += " -> ";
        m_components.for_each([&](auto& info) {
            ret += info.stringify();
            ret += " ";
        });
        ret += "\n";
        return ret;
    };

private:
    ExtendedInfo m_lhs;
    Vector<ExtendedInfo> m_components;
};

class ExtendedGrammar {
public:
    ExtendedGrammar(const Vector<std::shared_ptr<ItemSet>>& sets);
    ~ExtendedGrammar();

    const Vector<ExtendedRule>& rules() const { return m_rules; }

    String stringify() const {
        String ret = "";
        int i = 0;
        m_rules.for_each([&](auto& rule) {
            ret += rule.stringify(i++);
        });
        return ret;
    }

private:
    const Vector<std::shared_ptr<ItemSet>>& m_sets;
    Vector<ExtendedRule> m_rules;
};