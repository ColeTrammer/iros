#include <stdio.h>

#include "item_set.h"

static int item_set_number = 0;

Vector<std::shared_ptr<ItemSet>> ItemSet::create_item_sets(const Rule& start, const Vector<Rule>& rules,
                                                           const Vector<StringView>& token_types) {
    Vector<std::shared_ptr<ItemSet>> item_sets;
    Vector<std::shared_ptr<ItemSet>> to_process;

    auto create_from_rule_set_and_position = [&](const HashMap<Rule, bool>& rule_set, int position) -> std::shared_ptr<ItemSet> {
        Vector<Rule> to_expand;

        rule_set.for_each_key([&](auto& rule) {
            to_expand.add(rule);
        });

        auto set = std::make_shared<ItemSet>(rule_set, position);

        while (to_expand.size() != 0) {
            for (int size_save = to_expand.size() - 1; size_save >= 0; size_save--) {
                auto& rule_to_expand = to_expand.get(size_save);
                if (position < rule_to_expand.components().size()) {
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
    };

    auto add_item_set = [&](auto& item_set) {
        if (!item_sets.any_match([&](auto& other) {
                return *item_set == *other;
            })) {

            item_set->set_number(item_set_number++);
            item_sets.add(item_set);

            bool should_process = false;
            item_set->rules().for_each_key([&](auto& rule) {
                if (rule.components().size() > item_set->position()) {
                    should_process = true;
                }
            });

            if (should_process) {
                to_process.add(item_set);
            }
        }
    };

    HashMap<Rule, bool> first_set;
    first_set.put(start, true);
    auto first = create_from_rule_set_and_position(first_set, 0);
    add_item_set(first);

    while (to_process.size() != 0) {
        for (int size_save = to_process.size() - 1; size_save >= 0; size_save--) {
            auto& item_set_to_process = to_process.get(size_save);
            int position = item_set_to_process->position();

            auto process_identifier = [&](const StringView& identifier) {
                HashMap<Rule, bool> rule_set;

                item_set_to_process->rules().for_each_key([&](auto& rule) {
                    if (position < rule.components().size() && rule.components().get(position) == identifier) {
                        rule_set.put(rule, true);
                    }
                });

                bool sub_rule_used = false;

                item_set_to_process->set().for_each_key([&](auto& rule) {
                    if (0 < rule.components().size() && rule.components().get(0) == identifier) {
                        rule_set.put(rule, true);
                        sub_rule_used = true;
                    }
                });

                if (rule_set.empty()) {
                    return;
                }

                auto new_set = create_from_rule_set_and_position(rule_set, (sub_rule_used ? 0 : position) + 1);
                add_item_set(new_set);
            };

            token_types.for_each([&](auto& token) {
                process_identifier(token);
            });

            rules.for_each([&](auto& rule) {
                process_identifier(rule.name());
            });

            to_process.remove_element(item_set_to_process);
        }
    }

    return item_sets;
}

String ItemSet::stringify() const {
    String s = "";
    char buf[30];
    snprintf(buf, 30, "Item Set %d:\n", number());
    s += buf;
    rules().for_each_key([&](auto& rule) {
        s += "  ";
        s += rule.stringify(position());
        s += "\n";
    });
    m_set.for_each_key([&](auto& rule) {
        s += "  + ";
        s += rule.stringify(0);
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
