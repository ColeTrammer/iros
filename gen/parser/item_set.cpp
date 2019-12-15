#include <stdio.h>

#include "item_set.h"

static int item_set_number = 0;

Vector<std::shared_ptr<ItemSet>> ItemSet::create_item_sets(const Rule& start, const Vector<Rule>& rules,
                                                           const Vector<StringView>& token_types) {
    Vector<std::shared_ptr<ItemSet>> item_sets;
    auto first = std::make_shared<ItemSet>();
    first->set_number(item_set_number++);

    first->add_rule(start);

    auto expand_name = start.components().get(0);
    first->add_rule_name(rules, expand_name);

    first->set().for_each_key([&](auto& rule) {
        if (rule.components().size() > 0) {
            auto name = rule.components().get(0);
            if (!first->expanded().get(name)) {
                first->add_rule_name(rules, name);
            }
        }
    });

    item_sets.add(first);
    return item_sets;
}

String ItemSet::stringify() const {
    String s = "";
    char buf[30];
    snprintf(buf, 30, "Item Set %d\n", number());
    s += buf;
    m_set.for_each_key([&](auto& rule) {
        s += "  ";
        s += rule.stringify();
        s += "\n";
    });

    return s;
}

void ItemSet::add_rule_name(const Vector<Rule>& rules, const StringView& name) {
    rules.for_each([&](auto& rule) {
        if (rule.name() == name) {
            add_rule(rule);
        }
    });
}

void ItemSet::add_rule(const Rule& rule) {
    m_set.put(rule, true);
    m_expanded.put(rule.name(), true);
}
