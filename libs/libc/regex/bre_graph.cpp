#include <regex.h>
#include <stdio.h>

#include "bre_graph.h"

class EpsilonTransition final : public BRETransition {
public:
    EpsilonTransition(int state, bool forward) : BRETransition(state), m_forward(forward) {}

    virtual bool can_transition(const char* s, size_t i, int) const override { return m_forward || s[i] != '\0'; }

    virtual void dump() const override {
        fprintf(stderr, "  EpsilonTransition(forward=%s) to %d\n", m_forward ? "true" : "false", state());
    }

    virtual size_t num_characters_matched() const override { return 0; }

private:
    bool m_forward;
};

class LeftAnchorTransition final : public BRETransition {
public:
    LeftAnchorTransition(int state) : BRETransition(state) {}

    virtual bool can_transition(const char* s, size_t index, int flags) const override {
        return ((flags & REG_NEWLINE) && s[index] == '\n') || (index == 0 && !(flags & REG_NOTBOL));
    }

    virtual size_t num_characters_matched() const override { return 0; }

    virtual void dump() const override { fprintf(stderr, "  LeftAnchor to %d\n", state()); }
};

class RightAnchorTransition final : public BRETransition {
public:
    RightAnchorTransition(int state) : BRETransition(state) {}

    virtual bool can_transition(const char* s, size_t index, int flags) const override {
        return ((flags & REG_NEWLINE) && s[index] == '\n') || (s[index] == '\0' && !(flags & REG_NOTEOL));
    }

    virtual size_t num_characters_matched() const override { return 0; }

    virtual void dump() const override { fprintf(stderr, "  RightAnchor to %d\n", state()); }
};

class OrdinaryCharacterTransition final : public BRETransition {
public:
    OrdinaryCharacterTransition(int state, char to_match) : BRETransition(state), m_to_match(to_match) {}

    virtual bool can_transition(const char* s, size_t index, int flags) const override {
        if (flags & REG_ICASE) {
            return tolower(s[index] == tolower(m_to_match));
        }
        return s[index] == m_to_match;
    }

    virtual void dump() const override { fprintf(stderr, "  OrdinaryCharacterTransition (%c) to %d\n", m_to_match, state()); }

private:
    char m_to_match;
};

class AnyCharacterTransition final : public BRETransition {
public:
    AnyCharacterTransition(int state) : BRETransition(state) {}

    virtual bool can_transition(const char* s, size_t index, int flags) const override {
        return (!(flags & REG_NEWLINE) || s[index] != '\n') && s[index] != '\0';
    }

    virtual void dump() const override { fprintf(stderr, "  AnyCharacterTransition to %d\n", state()); }
};

BREGraph::BREGraph(const BRE& bre, int cflags) : m_cflags(cflags) {
    m_states.add(BREState());
    BREState* current_state = &m_states.last();

    auto add_forward_transition = [&]<typename Transition, typename... Args>(in_place_type_t<Transition>, Args... args) {
        current_state->transitions().add(make_shared<Transition>(m_states.size(), forward<Args>(args)...));
        m_states.add(BREState());
        current_state = &m_states.last();
    };

    auto add_epsilon_transition = [&](int from, int to) {
        if (from < to) {
            m_states[from].transitions().add(make_shared<EpsilonTransition>(to, true));
        } else {
            m_states[from].transitions().insert(make_shared<EpsilonTransition>(to, false), 0);
        }
    };

    if (bre.left_anchor) {
        add_forward_transition(in_place_type<LeftAnchorTransition>);
    }

    for (int i = 0; i < bre.expression.parts.size(); i++) {
        const SharedPtr<BRESingleExpression>& exp = bre.expression.parts[i];
        switch (exp->type) {
            case BRESingleExpression::Type::OrdinaryCharacter:
            case BRESingleExpression::Type::QuotedCharacter:
                assert(exp->expression.is<char>());
                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, exp->expression.as<char>());
                break;
            case BRESingleExpression::Type::Any:
                add_forward_transition(in_place_type<AnyCharacterTransition>);
                break;
            case BRESingleExpression::Type::BracketExpression:
            case BRESingleExpression::Type::Backreference:
            case BRESingleExpression::Type::Group:
                break;
        }

        if (exp->duplicate.has_value()) {
            const DuplicateCount& dup = exp->duplicate.value();
            switch (dup.type) {
                case DuplicateCount::Type::Star:
                    add_epsilon_transition(m_states.size() - 2, m_states.size());
                    add_epsilon_transition(m_states.size() - 1, m_states.size() - 2);
                    add_forward_transition(in_place_type<EpsilonTransition>, true);
                    break;
                case DuplicateCount::Type::AtLeast:
                case DuplicateCount::Type::Between:
                case DuplicateCount::Type::Exact:
                    break;
            }
        }
    }

    if (bre.right_anchor) {
        add_forward_transition(in_place_type<RightAnchorTransition>);
    }

    // Create a null rule so that the regex can be finished
    if (!current_state->transitions().empty()) {
        add_forward_transition(in_place_type<EpsilonTransition>, true);
    }
}

Maybe<size_t> BREGraph::try_match_at(const char* str, size_t index, int eflags, int state, Vector<regmatch_t>& dest_matches) const {
    const BREState& current_state = m_states[state];
    if (current_state.transitions().empty()) {
        return { index };
    }

    for (int i = 0; i < current_state.transitions().size(); i++) {
        const auto& trans = current_state.transitions()[i];
        if (!trans->can_transition(str, index, eflags | m_cflags)) {
            continue;
        }

        index += trans->transition(state);
        auto result = try_match_at(str, index, eflags, state, dest_matches);
        if (result.has_value()) {
            return result.value();
        }
    }

    return {};
}

Vector<regmatch_t> BREGraph::do_match(const char* str, int eflags) const {
    Vector<regmatch_t> matches(1);
    for (int i = 0; i < matches.capacity(); i++) {
        matches.add({ -1, -1 });
    }

    size_t i = 0;
    do {
        Maybe<size_t> result = try_match_at(str, i, eflags, 0, matches);
        if (result.has_value()) {
            matches[0] = { (regoff_t) i, (regoff_t) result.value() };
            break;
        }
    } while (str[i++] != '\0');

    return matches;
}

void BREGraph::dump() const {
    fprintf(stderr, "BREGraph::num_states() %d\n", m_states.size());
    for (int i = 0; i < m_states.size(); i++) {
        auto& state = m_states[i];
        fprintf(stderr, "State: %d (%d)\n", i, state.transitions().size());
        state.transitions().for_each([&](auto& tr) {
            tr->dump();
        });
    }
}