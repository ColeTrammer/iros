#include <assert.h>

#include "extended_grammar.h"

#define START_SET_DEBUG
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

static bool make_union(HashMap<StringView, bool>& s1, const HashMap<StringView, bool>& s2, bool log = false) {
    if (s2.empty()) {
        return false;
    }

    bool did_something = false;
    s2.for_each_key([&](const auto& key) {
        if (!s1.get(key)) {
            s1.put(key, true);
            did_something = true;
        }
    });

    return did_something;
}

void ExtendedGrammar::compute_first_sets() {
    HashMap<ExtendedInfo, int> done;
    HashMap<ExtendedInfo, int> rule_count;
    HashMap<ExtendedInfo, Vector<ExtendedInfo>> to_add;

    Vector<int> start_indexes;

    m_rules.for_each([&](const auto& rule) {
        start_indexes.add(0);

        if (!m_first_sets.get(rule.lhs())) {
            m_first_sets.put(rule.lhs(), std::make_shared<HashMap<StringView, bool>>());
        }

        if (!to_add.get(rule.lhs())) {
            to_add.put(rule.lhs(), Vector<ExtendedInfo>());
        }

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
    while (!all_done) {
        all_done = true;
        int current_index = -1;
        m_rules.for_each([&](ExtendedRule& rule) {
            current_index++;
            const auto& name = rule.lhs();
            if (is_done(name)) {
                return;
            }

            bool could_compute = true;
            auto& set = *m_first_sets.get(name);
            int& ended = start_indexes.get(current_index);
            if (ended == -1) {
                return;
            }

            bool every_contain_empty = true;
            for (int i = 0; i < rule.components().size(); i++) {
                const auto& rule_name = rule.components().get(i);
                if (rule_name == name) {
                    every_contain_empty = false;
                    break;
                }

                if (!is_done(rule_name)) {
                    could_compute = false;
                    break;
                }

                assert(m_first_sets.get(rule_name));
                HashMap<StringView, bool>& other_set = **m_first_sets.get(rule_name);

                if (!other_set.get("__Empty")) {
                    every_contain_empty = false;
                    break;
                }
                continue;
            }

            if (could_compute) {
                ended = -1;
                if (rule.components().size() == 0 || every_contain_empty) {
                    set->put("__Empty", true);
                }

                int* done_count = done.get(name);
                if (!done_count) {
                    done.put(name, 1);
#ifdef START_SET_DEBUG
                    fprintf(stderr, "Bump done count: %s [%d/%d]\n", name.stringify().string(), 1, rule_count.get_or(name, -1));
                    fprintf(stderr, "    Set: {");
                    set->for_each_key([&](const auto& st) {
                        fprintf(stderr, " %s", String(st).string());
                    });
                    fprintf(stderr, " }\n");
#endif /* START_SET_DEBUG */
                } else {
                    (*done_count)++;
#ifdef START_SET_DEBUG
                    fprintf(stderr, "Bump done count: %s [%d/%d]\n", name.stringify().string(), *done_count, rule_count.get_or(name, -1));
                    fprintf(stderr, "    Set: {");
                    set->for_each_key([&](const auto& st) {
                        fprintf(stderr, " %s", String(st).string());
                    });
                    fprintf(stderr, " }\n");
#endif /* START_SET_DEBUG */
                }
            } else {
                all_done = false;
            }
        });
    }

#ifdef START_SET_DEBUG
    fprintf(stderr, "Finished finding __Empty.\n");
#endif /* START_SET_DEBUG */

    m_rules.for_each([&](ExtendedRule& rule) {
        const auto& name = rule.lhs();
        for (int i = 0; i < rule.components().size(); i++) {
            const auto& part = rule.components().get(i);
            to_add.get(name)->add(part);
            if (!(*m_first_sets.get(part))->get("__Empty")) {
                break;
            }
        }
    });

    HashMap<ExtendedInfo, bool> was_updated;
    bool first_round = true;
    for (;;) {
        bool did_anything = false;
        HashMap<ExtendedInfo, bool> was_updated_this_round;
        to_add.for_each_key([&](const ExtendedInfo& info) {
            const Vector<ExtendedInfo>& need_to_add = *to_add.get(info);
            need_to_add.for_each([&](const ExtendedInfo& add) {
                bool* did_something_last_round = was_updated.get(add);
                if (first_round || did_something_last_round && *did_something_last_round) {
                    bool did_something = make_union(**m_first_sets.get(info), **m_first_sets.get(add));
                    if (did_something) {
                        was_updated_this_round.put(info, true);
                        did_anything = true;
                    }
                }
            });
        });

        if (!did_anything) {
            break;
        }

        first_round = false;
        was_updated = was_updated_this_round;
    }
}

void ExtendedGrammar::compute_follow_sets() {
    HashMap<ExtendedInfo, Vector<ExtendedInfo>> to_add;

    auto& first_rule = m_rules.get(0).lhs();
    m_follow_sets.put(first_rule, std::make_shared<HashMap<StringView, bool>>());
    (*m_follow_sets.get(first_rule))->put("End", true);

    Vector<int> last_processed;
    for (int i = 0; i < m_rules.size(); i++) {
        auto& rule = m_rules[i].lhs();
        if (!m_follow_sets.get(rule)) {
            m_follow_sets.put(rule, std::make_shared<HashMap<StringView, bool>>());
        }

        last_processed.add(0);
        Vector<ExtendedInfo> v;
        to_add.put(rule, v);
    }

    for (int i = 0; i < m_rules.size(); i++) {
        const auto& self = m_rules[i].lhs();
        for (; last_processed[i] < m_rules[i].components().size(); last_processed[i]++) {
            const auto& info = m_rules[i].components()[last_processed[i]];
            if (m_token_types.includes(info.name)) {
                continue;
            }

            if (last_processed[i] == m_rules[i].components().size() - 1) {
            add_self:
#ifdef FOLLOW_SET_DEBUG
                fprintf(stderr, "%s [%d of %d] finished with %s\n", info.stringify().string(), *done.get(info), *rule_count.get(info),
                        self.stringify().string());
#endif /* FOLLOW_SET_DEBUG */
                if (!to_add.get(info)->includes(self) && info != self) {
                    to_add.get(info)->add(self);
                }
                continue;
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
#ifdef FOLLOW_SET_DEBUG
                fprintf(stderr, "%s [%d of %d] finished with %s\n", info.stringify().string(), *done.get(info), *rule_count.get(info),
                        next.stringify().string());
#endif /* FOLLOW_SET_DEBUG */
                break;
            }
        }
    }

    HashMap<ExtendedInfo, bool> was_updated;
    bool first_round = true;
    for (;;) {
        bool did_anything = false;
        HashMap<ExtendedInfo, bool> was_updated_this_round;
        to_add.for_each_key([&](const ExtendedInfo& info) {
            const Vector<ExtendedInfo>& need_to_add = *to_add.get(info);
            need_to_add.for_each([&](const ExtendedInfo& add) {
                bool* did_something_last_round = was_updated.get(add);
                if (first_round || did_something_last_round && *did_something_last_round) {
                    bool did_something = make_union(**m_follow_sets.get(info), **m_follow_sets.get(add));
                    if (did_something) {
                        was_updated_this_round.put(info, true);
                        did_anything = true;
                    }
                }
            });
        });

        if (!did_anything) {
            break;
        }

        was_updated = was_updated_this_round;
        first_round = false;
    }
}

ExtendedGrammar::~ExtendedGrammar() {}