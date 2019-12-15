#include <stdio.h>

#include "item_set.h"

static int item_set_number = 0;

std::shared_ptr<ItemSet> ItemSet::create_from_rule_and_position(const Rule& rule, int position, const Vector<Rule>& rules,
                                                                const Vector<StringView>& token_types) {
    Vector<Rule> to_expand;

    auto set = std::make_shared<ItemSet>(rule);
    to_expand.add(rule);

    while (to_expand.size() != 0) {
        for (int size_save = to_expand.size() - 1; size_save >= 0; size_save--) {
            auto& rule_to_expand = to_expand.get(size_save);
            if (rule_to_expand.components().size() > position) {
                auto expand_name = rule_to_expand.components().get(position);
                if (!token_types.includes(expand_name) && !set->expanded().get(expand_name)) {
                    set->add_rule_name(rules, expand_name, to_expand);
                }
            }

            to_expand.remove_element(rule_to_expand);
        }

        position = 0;
    }

    return set;
}

Vector<std::shared_ptr<ItemSet>> ItemSet::create_item_sets(const Rule& start, const Vector<Rule>& rules,
                                                           const Vector<StringView>& token_types) {
    Vector<std::shared_ptr<ItemSet>> item_sets;
    Vector<std::shared_ptr<ItemSet>> to_process;

    auto add_item_set = [&](auto& item_set) {
        if (!item_sets.includes(item_set)) {
            item_set->set_number(item_set_number++);
            item_sets.add(item_set);
            to_process.add(item_set);
        }
    };

    auto first = ItemSet::create_from_rule_and_position(start, 0, rules, token_types);
    add_item_set(first);

    while (to_process.size() != 0) {
        for (int size_save = to_process.size() - 1; size_save >= 0; size_save--) {
            auto& item_set_to_process = to_process.get(size_save);

            auto process_identifier = [&](const auto& identifier) {
                item_set_to_process->set().for_each_key([&](auto& rule) {
                    if (rule.components().size() && rule.components().get(0) == identifier) {
                        auto new_set = ItemSet::create_from_rule_and_position(rule, 1, rules, token_types);
                        add_item_set(new_set);
                    }
                });
            };

            token_types.for_each([&](auto& token) {
                process_identifier(token);
            });

            rules.for_each([&](auto& rule) {
                process_identifier(rule.name());
            });

            to_process.remove_element(item_set_to_process);
        }

        break;
    }

    return item_sets;
}

String ItemSet::stringify() const {
    String s = "";
    char buf[30];
    snprintf(buf, 30, "Item Set %d:\n", number());
    s += buf;
    s += "  ";
    s += rule().stringify();
    s += "\n";
    m_set.for_each_key([&](auto& rule) {
        s += "  + ";
        s += rule.stringify();
        s += "\n";
    });

    return s;
}

void ItemSet::add_rule_name(const Vector<Rule>& rules, const StringView& name, Vector<Rule>& to_expand) {
    rules.for_each([&](auto& rule) {
        if (rule.name() == name) {
            add_rule(rule, to_expand);
        }
    });
}

void ItemSet::add_rule(const Rule& rule, Vector<Rule>& to_expand) {
    m_set.put(rule, true);
    m_expanded.put(rule.name(), true);
    to_expand.add(rule);
}
