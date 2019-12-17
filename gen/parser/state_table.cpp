#include "state_table.h"

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
        if (m_rules.get(new_rule.number())) {
            make_union(m_rules.get(new_rule.number())->follow_set(), new_rule.follow_set());
        } else {
            m_rules.put(new_rule.number(), new_rule);
        }
    });

    const Vector<std::shared_ptr<ItemSet>>& sets = grammar.sets();
    sets.for_each([&](const std::shared_ptr<ItemSet>& set) {
        HashMap<StringView, Action> m;
        m_table.add(m);
        auto& row = m_table.last();
        identifiers.for_each([&](const StringView& id) {
            int* jump_number = set->table().get(id);
            if (jump_number) {
                row.put(id, { token_types.includes(id) ? Action::Type::Shift : Action::Type::Jump, *jump_number });
                return;
            }

            const FinalRule* reduce_table = m_rules.get(set->number());
            if (!reduce_table) {
                return;
            }

            const bool* present = reduce_table->follow_set().get(id);
            if (!present || !*present) {
                return;
            }

            if (reduce_table->original_number() == 0) {
                row.put(id, { Action::Type::Accept, END_SET });
                return;
            }

            row.put(id, { Action::Type::Reduce, reduce_table->original_number() });
        });
    });
}

StateTable::~StateTable() {}

String StateTable::stringify() {
    String ret = String::format("State table\n%-6s", "#");

    m_identifiers.for_each([&](const auto& id) {
        ret += String::format(" %-6s", String(id).string());
    });
    ret += "\n";

#if 0
    m_rules.for_each([&](const auto& rule) {
        ret += rule.stringify();
    });
#endif

    for (int i = 0; i < m_table.size(); i++) {
        ret += String::format("%-6d", i);
        m_identifiers.for_each([&](const auto& id) {
            Action* action = m_table.get(i).get(id);
            ret += String::format(" %-6s", action ? action->stringify().string() : " ");
        });
        ret += "\n";
    }

    return ret;
}