#include <assert.h>

#include "extended_grammar.h"

// #define FOLLOW_SET_DEBUG

ExtendedGrammar::ExtendedGrammar(const Vector<std::shared_ptr<ItemSet>>& sets, const Vector<StringView>& token_types)
    : m_sets(sets), m_token_types(token_types) {
    auto first_set = m_sets.get(0);
    first_set->rules().for_each_key([&](const Rule& rule) {
        ExtendedRule e({ rule.name(), { 0, END_SET } }, rule.number());

        auto& name_to_follow = rule.components().get(0);
        e.components().add({ name_to_follow, { 0, *first_set->table().get(name_to_follow) } });

        m_rules.add(e);
    });

    sets.for_each([&](const std::shared_ptr<ItemSet>& set) {
        set->set().for_each_key([&](const Rule& rule) {
            ExtendedRule e({ rule.name(), { set->number(), *set->table().get(rule.name()) } }, rule.number());

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

    Vector<int> start_indexes;

    m_rules.for_each([&](const auto& rule) {
        start_indexes.add(0);

        int* count = rule_count.get(rule.lhs());
        if (!count) {
            rule_count.put(rule.lhs(), 1);
        } else {
            (*count)++;
        }

        rule.components().for_each([&](const ExtendedInfo& info) {
            if (m_token_types.includes(info.name)) {
                auto set1 = std::make_shared<HashMap<StringView, bool>>();
                set1->put(info.name, true);
                m_first_sets.put(info, set1);
                done.put(info, 1);
                rule_count.put(info, 1);
            }
        });
    });

    auto is_done = [&](const ExtendedInfo& info) -> bool {
        assert(rule_count.get(info));
        return done.get_or(info, 0) == rule_count.get_or(info, -1);
    };

    bool all_done = false;
    bool made_progress_last_round = true;
    while (!all_done) {
        all_done = true;
        int current_index = -1;
        bool made_progress = false;
        m_rules.for_each([&](ExtendedRule& rule) {
            current_index++;
            const auto& name = rule.lhs();
            if (is_done(name)) {
                return;
            }

            bool could_compute = true;
            auto set = std::make_shared<HashMap<StringView, bool>>();
            int& i = start_indexes.get(current_index);
            if (i == -1) {
                return;
            }
            for (; i < rule.components().size(); i++) {
                const auto& rule_name = rule.components().get(i);
                if (rule_name == name && !made_progress_last_round && m_first_sets.get(name)) {
                    if ((*m_first_sets.get(name))->get("__Empty")) {
                        continue;
                    }
                    set = *m_first_sets.get(name);
                    break;
                }

                if (!is_done(rule_name)) {
                    could_compute = false;
                    break;
                }

                assert(m_first_sets.get(rule_name));
                HashMap<StringView, bool>& other_set = **m_first_sets.get(rule_name);
                assert(!other_set.empty());

                other_set.for_each_key([&](auto& key) {
                    if (key != "__Empty" || i == rule.components().size() - 1) {
                        set->put(key, true);
                    }
                });

                if (other_set.get("__Empty")) {
                    continue;
                }
                break;
            }

            if (could_compute) {
                i = -1;
                made_progress = true;
                if (set->empty()) {
                    set->put("__Empty", true);
                }

                int* done_count = done.get(name);
                if (!done_count) {
                    done.put(name, 1);
                    m_first_sets.put(name, set);
                } else {
                    (*done_count)++;
                    make_union(**m_first_sets.get(name), *set);
                }
            } else {
                all_done = false;
            }
        });

        made_progress_last_round = made_progress;
    }
}

void ExtendedGrammar::compute_follow_sets() {
    HashMap<ExtendedInfo, int> done;
    HashMap<ExtendedInfo, int> rule_count;
    HashMap<ExtendedInfo, Vector<ExtendedInfo>> needed;

    auto& first_rule = m_rules.get(0).lhs();
    m_follow_sets.put(first_rule, std::make_shared<HashMap<StringView, bool>>());
    (*m_follow_sets.get(first_rule))->put("End", true);
    rule_count.put(first_rule, 0);

    Vector<int> last_processed;
    for (int i = 0; i < m_rules.size(); i++) {
        auto& rule = m_rules[i].lhs();
        if (!m_follow_sets.get(rule)) {
            m_follow_sets.put(rule, std::make_shared<HashMap<StringView, bool>>());
        }
        done.put(rule, 0);
        m_rules[i].components().for_each([&](const ExtendedInfo& info) {
            if (m_token_types.includes(info.name)) {
                return;
            }

            int* count = rule_count.get(info);
            if (!count) {
                rule_count.put(info, 1);
            } else {
                (*count)++;
            }
        });
        last_processed.add(0);

        Vector<ExtendedInfo> v;
        needed.put(rule, v);
    }

    auto is_done = [&](const auto& info) {
        assert(done.get(info));
        assert(rule_count.get(info));
        return *done.get(info) >= *rule_count.get(info);
    };

    assert(is_done(first_rule));

    bool all_done = false;
    while (!all_done) {
        all_done = true;
        for (int i = 0; i < m_rules.size(); i++) {
            const auto& self = m_rules[i].lhs();
            for (; last_processed[i] < m_rules[i].components().size(); last_processed[i]++) {
                const auto& info = m_rules[i].components()[last_processed[i]];
                if (m_token_types.includes(info.name)) {
                    continue;
                }

                if (last_processed[i] == m_rules[i].components().size() - 1) {
                add_self:
                    if (is_done(self)) {
                        make_union(**m_follow_sets.get(info), **m_follow_sets.get(self));
#ifdef FOLLOW_SET_DEBUG
                        fprintf(stderr, "%s [%d of %d] finished with %s\n", info.stringify().string(), *done.get(info),
                                *rule_count.get(info), self.stringify().string());
#endif /* FOLLOW_SET_DEBUG */
                        (*done.get(info))++;
                        continue;
                    } else {
#ifdef FOLLOW_SET_DEBUG
                        fprintf(stderr, "%s [%d of %d] needs %s\n", info.stringify().string(), *done.get(info), *rule_count.get(info),
                                self.stringify().string());
#endif /* FOLLOW_SET_DEBUG */
                        if (!needed.get(info)->includes(self)) {
                            needed.get(info)->add(self);
                        }
                        break;
                    }
                }

                for (int j = last_processed[i] + 1; j <= m_rules[i].components().size(); j++) {
                    if (j == m_rules[i].components().size()) {
                        goto add_self;
                    }

                    const auto& next = m_rules[i].components()[j];
                    bool contains_empty = !!(*m_first_sets.get(next))->get("__Empty");
                    make_union(**m_follow_sets.get(info), **m_first_sets.get(next));
                    if (contains_empty) {
                        (*m_follow_sets.get(info))->remove("__Empty");
                        continue;
                    }
                    (*done.get(info))++;
#ifdef FOLLOW_SET_DEBUG
                    fprintf(stderr, "%s [%d of %d] finished with %s\n", info.stringify().string(), *done.get(info), *rule_count.get(info),
                            next.stringify().string());
#endif /* FOLLOW_SET_DEBUG */
                    break;
                }
            }

            if (last_processed[i] < m_rules[i].components().size()) {
                all_done = false;
            }
        }

        m_rules.for_each([&](auto& rule) {
            needed.get(rule.lhs())->remove_if([&](auto& other) -> bool {
                // Detect circular dependency
                return needed.get(other)->remove_if([&](auto& part) -> bool {
                    if (rule.lhs() == part) {
#ifdef FOLLOW_SET_DEBUG
                        fprintf(stderr, "Removing circular dependency: %s on %s\n", rule.lhs().stringify().string(),
                                other.stringify().string());
#endif /* FOLLOW_SET_DEBUG */
                        make_union(**m_follow_sets.get(rule.lhs()), **m_follow_sets.get(other));
                        make_union(**m_follow_sets.get(other), **m_follow_sets.get(rule.lhs()));
                        (*done.get(other))++;
                        (*done.get(rule.lhs()))++;
                        return true;
                    }

                    return false;
                });
            });
        });
    }
}

ExtendedGrammar::~ExtendedGrammar() {}