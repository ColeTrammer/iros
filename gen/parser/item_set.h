#pragma once

#include <liim/hash_map.h>
#include <liim/string_view.h>
#include <liim/traits.h>
#include <memory>

#include "rule.h"

class ItemSet {
public:
    static Vector<std::shared_ptr<ItemSet>> create_item_sets(const Rule& start, const Vector<Rule>& rules,
                                                             const Vector<StringView>& token_types);

    int number() const { return m_number; }

    HashMap<Rule, bool>& set() { return m_set; }
    const HashMap<Rule, bool>& set() const { return m_set; }

    HashMap<StringView, bool>& expanded() { return m_expanded; }
    const HashMap<StringView, bool>& expanded() const { return m_expanded; }

    bool operator==(const ItemSet& other) const { return this->set() == other.set(); }

    String stringify() const;

    ItemSet() {};
    ItemSet(const ItemSet& other) : m_expanded(other.m_expanded), m_set(other.m_set), m_number(other.number()) {};

    ~ItemSet() {}

private:
    void set_number(int n) { m_number = n; }

    void add_rule_name(const Vector<Rule>& rules, const StringView& name);
    void add_rule(const Rule& rule);

    HashMap<StringView, bool> m_expanded;
    HashMap<Rule, bool> m_set;
    int m_number { 0 };
};