#include <stdio.h>

#include "item_set.h"

static int item_set_number = 0;

Vector<SharedPtr<ItemSet>> ItemSet::create_item_sets(const Rule& start, const Vector<Rule>& rules, const Vector<StringView>& token_types) {
    Vector<SharedPtr<ItemSet>> item_sets;
    Vector<SharedPtr<ItemSet>> to_process;

    auto create_from_rule_set = [&](const HashMap<Rule, bool>& rule_set) -> SharedPtr<ItemSet> {
        Vector<Rule> to_expand;

        rule_set.for_each_key([&](auto& rule) {
            to_expand.add(rule);
        });

        auto set = make_shared<ItemSet>(rule_set);

        while (to_expand.size() != 0) {
            for (int size_save = to_expand.size() - 1; size_save >= 0; size_save--) {
                auto& rule_to_expand = to_expand.get(size_save);
                if (rule_to_expand.position() < rule_to_expand.components().size()) {
                    auto expand_name = rule_to_expand.components().get(rule_to_expand.position());
                    if (!token_types.includes(expand_name) && !set->expanded().get(expand_name)) {
                        set->add_rule_name(rules, expand_name, to_expand);
                    }
                }

                to_expand.remove_element(rule_to_expand);
            }
        }

        return set;
    };

    auto add_item_set = [&](auto& item_set) {
        auto* first_match = item_sets.first_match([&](auto& other) {
            return *item_set == *other;
        });

        if (!first_match) {
            item_set->set_number(item_set_number++);
            item_sets.add(item_set);

            bool should_process = false;
            item_set->rules().for_each_key([&](auto& rule) {
                if (rule.components().size() > rule.position()) {
                    should_process = true;
                }
            });

            if (should_process) {
                to_process.add(item_set);
            }
        } else {
            item_set->set_number((*first_match)->number());
        }
    };

    HashMap<Rule, bool> first_set;
    first_set.put(start, true);
    auto first = create_from_rule_set(first_set);
    add_item_set(first);

    while (to_process.size() != 0) {
        for (int size_save = to_process.size() - 1; size_save >= 0; size_save--) {
            auto& item_set_to_process = to_process.get(size_save);

            auto process_identifier = [&](const StringView& identifier) {
                HashMap<Rule, bool> rule_set;

                item_set_to_process->rules().for_each_key([&](auto& rule) {
                    if (rule.position() < rule.components().size() && rule.components().get(rule.position()) == identifier) {
                        Rule to_add(rule);
                        to_add.increment_position();
                        rule_set.put(to_add, true);
                    }
                });

                item_set_to_process->set().for_each_key([&](auto& rule) {
                    if (0 < rule.components().size() && rule.components().get(0) == identifier) {
                        Rule to_add(rule);
                        to_add.increment_position();
                        rule_set.put(to_add, true);
                    }
                });

                if (rule_set.empty()) {
                    return;
                }

                auto new_set = create_from_rule_set(rule_set);
                add_item_set(new_set);
                item_set_to_process->table().put(identifier, new_set->number());
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
        s += rule.stringify();
        s += "\n";
    });
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
