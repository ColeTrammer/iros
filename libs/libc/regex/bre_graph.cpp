#include <regex.h>
#include <stdio.h>

#include "bre_graph.h"

class EpsilonTransition final : public BRETransition {
public:
    EpsilonTransition(int state) : BRETransition(state) {}

    virtual bool can_transition(const char*, size_t, int) const override { return true; }

    virtual void dump() const override { fprintf(stderr, "  EpsilonTransition to %d\n", state()); }
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

    if (bre.left_anchor) {
        add_forward_transition(in_place_type<LeftAnchorTransition>);
    }

    bre.expression.parts.for_each([&](const SharedPtr<BRESingleExpression>& exp) {
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
    });

    if (bre.right_anchor) {
        add_forward_transition(in_place_type<RightAnchorTransition>);
    }

    dump();
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