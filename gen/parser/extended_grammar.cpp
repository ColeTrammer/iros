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
    compute_follow_sets();
}

static void make_union(HashMap<StringView, bool>& s1, const HashMap<StringView, bool>& s2) {
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
}

void ExtendedGrammar::compute_first_sets() {
    HashMap<ExtendedInfo, int> done;
    HashMap<ExtendedInfo, int> rule_count;

    m_rules.for_each([&](const auto& rule) {
        int* count = rule_count.get(rule.lhs());
        if (!count) {
            rule_count.put(rule.lhs(), 1);
        } else {
            (*count)++;
        }

        rule.components().for_each([&](const ExtendedInfo& info) {
            if (m_token_types.includes(info.name)) {
                HashMap<StringView, bool> set1;
                set1.put(info.name, true);
                m_first_sets.put(info, set1);
                done.put(info, 1);
                rule_count.put(info, 1);
            }
        });
    });

    auto is_done = [&](const ExtendedInfo& info) -> bool {
        return done.get_or(info, 0) == rule_count.get_or(info, -1);
    };

    bool all_done = false;
    while (!all_done) {
        all_done = true;
        m_rules.for_each([&](ExtendedRule& rule) {
            const auto& name = rule.lhs();
            if (is_done(name)) {
                return;
            }

            bool could_compute = true;
            HashMap<StringView, bool> set;
            for (int i = 0; i < rule.components().size(); i++) {
                const auto& rule_name = rule.components().get(i);
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

void ExtendedGrammar::compute_follow_sets() {
#if 0
    auto& first_rule = m_rules.get(0).lhs().name;
    HashMap<StringView, bool> first_follow_set;
    first_follow_set.put("$", true);
    m_follow_sets.put(first_rule, first_follow_set);
#endif
}

ExtendedGrammar::~ExtendedGrammar() {}