#include "state_table.h"

#define REDUCE_RULES_DEBUG

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

StateTable::StateTable(const ExtendedGrammar& grammar, const Vector<StringView>& identifiers, const Vector<StringView>& token_types)
    : m_grammar(grammar), m_identifiers(identifiers) {
    m_grammar.rules().for_each([&](auto& rule) {
        FinalRule new_rule(rule, **m_grammar.follow_sets().get(rule.lhs()));
        auto* rules = m_rules.get(new_rule.number());
        if (rules) {
            auto* to_merge = rules->first_match([&](const auto& t) {
                return t == new_rule;
            });
            if (to_merge) {
                make_union(to_merge->follow_set(), new_rule.follow_set());
            } else {
                rules->add(new_rule);
            }
        } else {
            Vector<FinalRule> to_add;
            to_add.add(new_rule);
            m_rules.put(new_rule.number(), to_add);
        }
    });

    const Vector<SharedPtr<ItemSet>>& sets = grammar.sets();
    sets.for_each([&](const SharedPtr<ItemSet>& set) {
        HashMap<StringView, Action> m;
        m_table.add(m);
        auto& row = m_table.last();
        identifiers.for_each([&](const StringView& id) {
            const int* jump_number = set->table().get(id);
            if (jump_number) {
                row.put(id, { token_types.includes(id) ? Action::Type::Shift : Action::Type::Jump, *jump_number });
                return;
            }

            const Vector<FinalRule>* reduce_table = m_rules.get(set->number());
            if (!reduce_table) {
                return;
            }

            const FinalRule* match = reduce_table->first_match([&](const FinalRule& rule) {
                return !!rule.follow_set().get(id);
            });

            if (!match) {
                return;
            }

            if (match->original_number() == 0) {
                row.put(id, { Action::Type::Accept, END_SET });
                return;
            }

            row.put(id, { Action::Type::Reduce, match->original_number() });
        });
    });
}

StateTable::~StateTable() {}

String StateTable::stringify() {
    String ret = String::format("State table\n%-12s", "#");

    m_identifiers.for_each([&](const auto& id) {
        ret += String::format(" %-12s", String(id).string());
    });
    ret += "\n";

#ifdef REDUCE_RULES_DEBUG
    ret += "------------------------------------------\n";
    m_rules.for_each([&](const auto& rules) {
        rules.for_each([&](const auto& rule) {
            ret += rule.stringify();
        });
    });
    ret += "------------------------------------------\n";
#endif /* START_SET_DEBUG */

    for (int i = 0; i < m_table.size(); i++) {
        ret += String::format("%-12d", i);
        m_identifiers.for_each([&](const auto& id) {
            Action* action = m_table.get(i).get(id);
            ret += String::format(" %-12s", action ? action->stringify().string() : " ");
        });
        ret += "\n";
    }

    return ret;
}