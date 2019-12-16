#include <assert.h>

#include "extended_grammar.h"

ExtendedGrammar::ExtendedGrammar(const Vector<std::shared_ptr<ItemSet>>& sets, const Vector<StringView>& token_types)
    : m_sets(sets), m_token_types(token_types) {
    auto first_set = m_sets.get(0);
    first_set->rules().for_each_key([&](const Rule& rule) {
        ExtendedRule e({ rule.name(), { 0, END_SET } });

        auto& name_to_follow = rule.components().get(0);
        e.components().add({ name_to_follow, { 0, *first_set->table().get(name_to_follow) } });

        m_rules.add(e);
    });

    sets.for_each([&](const std::shared_ptr<ItemSet>& set) {
        set->set().for_each_key([&](const Rule& rule) {
            ExtendedRule e({ rule.name(), { set->number(), *set->table().get(rule.name()) } });

            int start_set = set->number();
            rule.components().for_each([&](const StringView& part) {
                int end_set = *m_sets.get(start_set)->table().get(part);

                e.components().add({ part, { start_set, end_set } });
                start_set = end_set;
            });

            m_rules.add(e);
        });
    });

    compute_first_sets();
}

void ExtendedGrammar::compute_first_sets() {
    auto make_union = [](HashMap<StringView, bool>& s1, const HashMap<StringView, bool>& s2) {
        if (s1.empty()) {
            s1 = s2;
        } else if (s2.empty()) {
            return;
        }

        s2.for_each_key([&](const auto& key) {
            if (!s1.get(key)) {
                s1.put(key, true);
            }
        });
    };

    HashMap<StringView, int> done;
    HashMap<StringView, int> rule_count;

    m_token_types.for_each([&](const StringView& token) {
        HashMap<StringView, bool> set1;
        set1.put(token, true);
        m_first_sets.put(token, set1);
        done.put(token, 1);
        rule_count.put(token, 1);
        assert(!m_first_sets.get(token)->empty());
    });

    m_rules.for_each([&](const auto& rule) {
        int* count = rule_count.get(rule.lhs().name);
        if (!count) {
            rule_count.put(rule.lhs().name, 1);
        } else {
            (*count)++;
        }
    });

    auto is_done = [&](const StringView& name) -> bool {
        return done.get_or(name, 0) == rule_count.get_or(name, -1);
    };

    bool all_done = false;
    while (!all_done) {
        all_done = true;
        m_rules.for_each([&](ExtendedRule& rule) {
            const auto& name = rule.lhs().name;
            if (is_done(name)) {
                return;
            }

            bool could_compute = true;
            HashMap<StringView, bool> set;
            for (int i = 0; i < rule.components().size(); i++) {
                const auto& rule_name = rule.components().get(i).name;
                if (!is_done(rule_name)) {
                    could_compute = false;
                    break;
                }

                HashMap<StringView, bool>& other_set = *m_first_sets.get(rule_name);
                if (other_set.empty()) {
                    continue;
                }

                other_set.for_each_key([&](auto& key) {
                    set.put(key, true);
                });
                break;
            }

            if (could_compute) {
                int* done_count = done.get(name);
                if (!done_count) {
                    done.put(name, 1);
                    m_first_sets.put(name, set);
                } else {
                    (*done_count)++;
                    make_union(*m_first_sets.get(name), set);
                }
            } else {
                all_done = false;
            }
        });
    }
}

ExtendedGrammar::~ExtendedGrammar() {}