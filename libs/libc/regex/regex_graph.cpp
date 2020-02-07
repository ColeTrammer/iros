#include <liim/function.h>
#include <regex.h>
#include <stdio.h>

#include "regex_graph.h"

class EpsilonTransition final : public RegexTransition {
public:
    EpsilonTransition(int state, bool forward) : RegexTransition(state), m_forward(forward) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t i, int, Vector<regmatch_t>&) const override {
        if (m_forward || s[i] != '\0')
            return { 0 };
        return {};
    }

    virtual void dump() const override {
        fprintf(stderr, "  EpsilonTransition(forward=%s) to %d\n", m_forward ? "true" : "false", state());
    }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const {
        return make_shared<EpsilonTransition>(state() + shift, m_forward);
    }

private:
    bool m_forward;
};

class LeftAnchorTransition final : public RegexTransition {
public:
    LeftAnchorTransition(int state) : RegexTransition(state) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int flags, Vector<regmatch_t>&) const override {
        if (((flags & REG_NEWLINE) && s[index] == '\n') || (index == 0 && !(flags & REG_NOTBOL)))
            return { 0 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  LeftAnchor to %d\n", state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<LeftAnchorTransition>(state() + shift);
    }
};

class RightAnchorTransition final : public RegexTransition {
public:
    RightAnchorTransition(int state) : RegexTransition(state) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int flags, Vector<regmatch_t>&) const override {
        if (((flags & REG_NEWLINE) && s[index] == '\n') || (s[index] == '\0' && !(flags & REG_NOTEOL)))
            return { 0 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  RightAnchor to %d\n", state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<RightAnchorTransition>(state() + shift);
    }
};

class OrdinaryCharacterTransition final : public RegexTransition {
public:
    OrdinaryCharacterTransition(int state, char to_match) : RegexTransition(state), m_to_match(to_match) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int flags, Vector<regmatch_t>&) const override {
        if (flags & REG_ICASE) {
            if (tolower(s[index] == tolower(m_to_match)))
                return { 1 };
            return {};
        }
        if (s[index] == m_to_match)
            return { 1 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  OrdinaryCharacterTransition (%c) to %d\n", m_to_match, state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<OrdinaryCharacterTransition>(state() + shift, m_to_match);
    }

private:
    char m_to_match;
};

class AnyCharacterTransition final : public RegexTransition {
public:
    AnyCharacterTransition(int state) : RegexTransition(state) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int flags, Vector<regmatch_t>&) const override {
        if ((!(flags & REG_NEWLINE) || s[index] != '\n') && s[index] != '\0')
            return { 1 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  AnyCharacterTransition to %d\n", state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<AnyCharacterTransition>(state() + shift);
    }
};

class BeginGroupCaptureTransition final : public RegexTransition {
public:
    BeginGroupCaptureTransition(int state, int group) : RegexTransition(state), m_group(group) {}

    virtual Maybe<size_t> do_try_transition(const char*, size_t index, int, Vector<regmatch_t>& matches) const override {
        matches[m_group].rm_so = index;
        return { 0 };
    }

    virtual void dump() const override { fprintf(stderr, "  BeginGroupCaptureTransition (%d) to %d\n", m_group, state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<BeginGroupCaptureTransition>(state() + shift, m_group);
    }

private:
    int m_group;
};

class EndGroupCaptureTransition final : public RegexTransition {
public:
    EndGroupCaptureTransition(int state, int group) : RegexTransition(state), m_group(group) {}

    virtual Maybe<size_t> do_try_transition(const char*, size_t index, int, Vector<regmatch_t>& matches) const override {
        matches[m_group].rm_eo = index;
        return { 0 };
    }

    virtual void dump() const override { fprintf(stderr, "  EndGroupCaptureTransition (%d) to %d\n", m_group, state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<EndGroupCaptureTransition>(state() + shift, m_group);
    }

private:
    int m_group;
};

RegexGraph::RegexGraph(const ParsedRegex& regex_base, int cflags, int num_groups) : m_cflags(cflags), m_num_groups(num_groups) {
    m_states.add(RegexState());
    RegexState* current_state = &m_states.last();

    auto add_forward_transition = [&]<typename Transition, typename... Args>(in_place_type_t<Transition>, Args... args) {
        current_state->transitions().add(make_shared<Transition>(m_states.size(), forward<Args>(args)...));
        m_states.add(RegexState());
        current_state = &m_states.last();
    };

    auto add_epsilon_transition = [&](int from, int to) {
        if (from < to) {
            m_states[from].transitions().add(make_shared<EpsilonTransition>(to, true));
        } else {
            m_states[from].transitions().insert(make_shared<EpsilonTransition>(to, false), 0);
        }
    };

    auto duplicate_states_after = [&](int start_state, int times_to_do_so) -> int {
        m_states.remove_last();
        int last_state = m_states.size() - 1;
        int num_states_added_each_time = last_state - start_state + 1;
        int shifted_start = start_state;
        for (int i = 0; i < times_to_do_so; i++) {
            shifted_start = m_states.size();
            for (int state = start_state; state <= last_state; state++) {
                RegexState state_to_add = RegexState::copy_with_shift(m_states[state], num_states_added_each_time * (i + 1));
                m_states.add(move(state_to_add));
            }
        }
        m_states.add(RegexState());
        current_state = &m_states.last();
        return shifted_start;
    };

    Function<void(const ParsedRegex&)> build_graph = [&](const ParsedRegex& regex) {
        auto expression = regex.alternatives.first();
        for (int i = 0; i < expression.parts.size(); i++) {
            int start_state = m_states.size() - 1;

            const SharedPtr<RegexSingleExpression>& exp = expression.parts[i];
            switch (exp->type) {
                case RegexSingleExpression::Type::OrdinaryCharacter:
                case RegexSingleExpression::Type::QuotedCharacter:
                    assert(exp->expression.is<char>());
                    add_forward_transition(in_place_type<OrdinaryCharacterTransition>, exp->expression.as<char>());
                    break;
                case RegexSingleExpression::Type::Any:
                    add_forward_transition(in_place_type<AnyCharacterTransition>);
                    break;
                case RegexSingleExpression::Type::Group: {
                    const auto& sub_regex = exp->expression.as<ParsedRegex>();
                    add_forward_transition(in_place_type<BeginGroupCaptureTransition>, sub_regex.index);
                    build_graph(sub_regex);
                    add_forward_transition(in_place_type<EndGroupCaptureTransition>, exp->expression.as<ParsedRegex>().index);
                    break;
                }
                case RegexSingleExpression::Type::Backreference:
                case RegexSingleExpression::Type::BracketExpression:
                    break;
            }

            if (exp->duplicate.has_value()) {
                const DuplicateCount& dup = exp->duplicate.value();
                switch (dup.type) {
                    case DuplicateCount::Type::Exact:
                        if (dup.min != 0) {
                            duplicate_states_after(start_state, dup.min - 1);
                        }
                        break;
                    case DuplicateCount::Type::AtLeast:
                        if (dup.min == 0) {
                            add_epsilon_transition(start_state, m_states.size());
                        } else {
                            start_state = duplicate_states_after(start_state, dup.min - 1);
                        }
                        add_epsilon_transition(m_states.size() - 1, start_state);
                        add_forward_transition(in_place_type<EpsilonTransition>, true);
                        break;
                    case DuplicateCount::Type::Between:
                        if (dup.max != 0) {
                            int num_states_in_part = m_states.size() - 1 - start_state;
                            duplicate_states_after(start_state, dup.max - 1);
                            for (int j = dup.min; j < dup.max; j++) {
                                add_epsilon_transition(start_state + j * num_states_in_part, m_states.size() - 1);
                            }
                        }
                        break;
                }
            }
        }

        // Create a null rule so that the regex can be finished
        if (!current_state->transitions().empty()) {
            add_forward_transition(in_place_type<EpsilonTransition>, true);
        }
    };

    build_graph(regex_base);
}

Maybe<size_t> RegexGraph::try_match_at(const char* str, size_t index, int eflags, int state, Vector<regmatch_t>& dest_matches) const {
    const RegexState& current_state = m_states[state];
    if (current_state.transitions().empty()) {
        return { index };
    }

    for (int i = 0; i < current_state.transitions().size(); i++) {
        const auto& trans = current_state.transitions()[i];
        if (!trans->try_transition(str, index, eflags | m_cflags, state, index, dest_matches)) {
            continue;
        }

        auto result = try_match_at(str, index, eflags, state, dest_matches);
        if (result.has_value()) {
            return result.value();
        }
    }

    return {};
}

Maybe<Vector<regmatch_t>> RegexGraph::do_match(const char* str, int eflags) const {
    Vector<regmatch_t> matches(m_num_groups + 1);
    for (int i = 0; i < matches.capacity(); i++) {
        matches.add({ -1, -1 });
    }

    size_t i = 0;
    do {
        Maybe<size_t> result = try_match_at(str, i, eflags, 0, matches);
        if (result.has_value()) {
            matches[0] = { (regoff_t) i, (regoff_t) result.value() };
            for (int i = 0; i < matches.size(); i++) {
                if (matches[i].rm_eo == -1 || matches[i].rm_so == -1) {
                    matches[i] = { -1, -1 };
                }
            }

            return matches;
        }
    } while (str[i++] != '\0');

    return {};
}

void RegexGraph::dump() const {
    fprintf(stderr, "RegexGraph::num_states() %d\n", m_states.size());
    for (int i = 0; i < m_states.size(); i++) {
        auto& state = m_states[i];
        fprintf(stderr, "State: %d (%d)\n", i, state.transitions().size());
        state.transitions().for_each([&](auto& tr) {
            tr->dump();
        });
    }
}