#include <ctype.h>
#include <liim/function.h>
#include <stdio.h>

#ifdef USERLAND_NATIVE
#include "../include/regex.h"
#else
#include <regex.h>
#endif /* USERLAND_NATIVE */

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
            if (tolower(s[index]) == tolower(m_to_match))
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

class SpaceCharacterTransition final : public RegexTransition {
public:
    SpaceCharacterTransition(int state, bool inverted) : RegexTransition(state), m_inverted(inverted) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int, Vector<regmatch_t>&) const override {
        char ch = s[index];
        if (ch == '\0') {
            return {};
        }

        if (isspace(ch) ^ m_inverted)
            return { 1 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  SpaceCharacterTransition (\\%c) to %d\n", m_inverted ? 'S' : 's', state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<SpaceCharacterTransition>(state() + shift, m_inverted);
    }

private:
    bool m_inverted;
};

class WordCharacterTransition final : public RegexTransition {
public:
    WordCharacterTransition(int state, bool inverted) : RegexTransition(state), m_inverted(inverted) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int, Vector<regmatch_t>&) const override {
        char ch = s[index];
        if (ch == '\0') {
            return {};
        }

        if ((isalnum(ch) || ch == '_') ^ m_inverted)
            return { 1 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  WordCharacterTransition (\\%c) to %d\n", m_inverted ? 'W' : 'w', state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<WordCharacterTransition>(state() + shift, m_inverted);
    }

private:
    bool m_inverted;
};

class DigitCharacterTransition final : public RegexTransition {
public:
    DigitCharacterTransition(int state, bool inverted) : RegexTransition(state), m_inverted(inverted) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int, Vector<regmatch_t>&) const override {
        char ch = s[index];
        if (ch == '\0') {
            return {};
        }

        if (isdigit(ch) ^ m_inverted)
            return { 1 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  DigitCharacterTransition (\\%c) to %d\n", m_inverted ? 'D' : 'd', state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<DigitCharacterTransition>(state() + shift, m_inverted);
    }

private:
    bool m_inverted;
};

class WordBoundaryTransition final : public RegexTransition {
public:
    WordBoundaryTransition(int state, bool inverted) : RegexTransition(state), m_inverted(inverted) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int, Vector<regmatch_t>&) const override {
        char ch = s[index];
        if (ch == '\0' || index == 0) {
            if (!m_inverted)
                return { 0 };
            return {};
        }

        char prev = s[index - 1];
        bool is_prev_word = isalpha(prev) || prev == '_';
        bool is_curr_word = isalpha(ch) || ch == '_';
        if ((is_prev_word != is_curr_word) ^ m_inverted)
            return { 0 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  WordBoundaryTransition (\\%c) to %d\n", m_inverted ? 'B' : 'b', state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<WordBoundaryTransition>(state() + shift, m_inverted);
    }

private:
    bool m_inverted;
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

class BackreferenceTransition final : public RegexTransition {
public:
    BackreferenceTransition(int state, int group) : RegexTransition(state), m_group(group) {}

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int, Vector<regmatch_t>& matches) const override {
        regmatch_t match = matches[m_group];
        if (match.rm_eo == -1 || match.rm_so == -1) {
            return {};
        }

        // FIXME: use some highly advanced string searching algorithm
        //        instead of a simple for loop
        size_t start_index = index;
        for (; match.rm_so < match.rm_eo; match.rm_so++) {
            if (s[index++] != s[match.rm_so]) {
                return {};
            }
        }

        return { index - start_index };
    }

    virtual void dump() const override { fprintf(stderr, "  BackreferenceTransition (%d) to %d", m_group, state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<BackreferenceTransition>(state() + shift, m_group);
    }

private:
    int m_group;
};

class BracketItemMatcher {
public:
    virtual Maybe<size_t> matches(const char* s, size_t index, int flags) const = 0;

    virtual ~BracketItemMatcher() {}
};

class BracketRangeMatcher final : public BracketItemMatcher {
public:
    BracketRangeMatcher(StringView a, StringView b, int&) {
        // FIXME: consider multi character collating sequences
        char c = a.first();
        char d = b.first();
        min = LIIM::min(c, d);
        max = LIIM::max(c, d);
    }

    virtual Maybe<size_t> matches(const char* s, size_t index, int flags) const override {
        // FIXME: fix non linear collating sequences
        if (flags & REG_ICASE) {
            if (tolower(s[index]) >= tolower(min) && tolower(s[index]) <= tolower(max))
                return { 1 };
            return {};
        }
        if (s[index] >= min && s[index] <= max)
            return { 1 };
        return {};
    }

private:
    char min;
    char max;
};

class BracketSingleCollateMatcher final : public BracketItemMatcher {
public:
    BracketSingleCollateMatcher(StringView sv, int&) { to_match = sv[0]; }

    virtual Maybe<size_t> matches(const char* s, size_t index, int flags) const override {
        if (flags & REG_ICASE) {
            if (tolower(s[index]) == tolower(to_match))
                return { 1 };
            return {};
        }
        if (s[index] == to_match)
            return { 1 };
        return {};
    }

private:
    char to_match;
};

class BracketEquivalenceClassMatcher final : public BracketItemMatcher {
public:
    BracketEquivalenceClassMatcher(StringView sv, int&) { to_match = sv.first(); }

    virtual Maybe<size_t> matches(const char* s, size_t index, int flags) const override {
        // FIXME: what even are equivalence classes anyway?
        if (flags & REG_ICASE) {
            if (tolower(s[index]) == tolower(to_match))
                return { 1 };
            return {};
        }
        if (s[index] == to_match)
            return { 1 };
        return {};
    }

private:
    char to_match;
};

class BracketCharacterClassMatcher final : public BracketItemMatcher {
public:
    BracketCharacterClassMatcher(StringView sv, int& error) {
        if (sv == "upper")
            match_function = isupper;
        else if (sv == "lower")
            match_function = islower;
        else if (sv == "alpha")
            match_function = isalpha;
        else if (sv == "digit")
            match_function = isdigit;
        else if (sv == "xdigit")
            match_function = isxdigit;
        else if (sv == "alnum")
            match_function = isalnum;
        else if (sv == "punct")
            match_function = ispunct;
        else if (sv == "blank")
            match_function = isblank;
        else if (sv == "space")
            match_function = isspace;
        else if (sv == "cntrl")
            match_function = iscntrl;
        else if (sv == "graph")
            match_function = isgraph;
        else if (sv == "print")
            match_function = isprint;
        else
            error = REG_ECTYPE;
    }

    virtual Maybe<size_t> matches(const char* s, size_t index, int) const override {
        // FIXME: Should [:upper:] or [:lower:] always match if REG_ICASE is set?
        if (match_function(s[index]))
            return { 1 };
        return {};
    }

private:
    int (*match_function)(int);
};

class BracketExpressionTransition final : public RegexTransition {
private:
    enum class Badge { Construct };

public:
    static Variant<int, SharedPtr<RegexTransition>> try_construct(int state, const BracketExpression& exp) {
        int error = 0;
        auto ptr = make_shared<BracketExpressionTransition>(state, exp, error, Badge::Construct);
        if (error != 0) {
            return { error };
        }
        return { move(static_cast<SharedPtr<RegexTransition>>(ptr)) };
    }

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int flags, Vector<regmatch_t>&) const override {
        // We shouldn't be able to match the null terminator, even when the bracket expression is inverted.
        if (s[index] == '\0') {
            return {};
        }

        for (int i = 0; i < m_components.size(); i++) {
            const auto& matcher = m_components[i];
            auto result = matcher->matches(s, index, flags);
            if (result.has_value()) {
                if (m_inverted)
                    return {};
                return result;
            }
        }

        if (m_inverted)
            return { 1 };
        return {};
    }

    virtual void dump() const override { fprintf(stderr, "  BracketExpression to %d", state()); }

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const override {
        return make_shared<BracketExpressionTransition>(state() + shift, m_components, m_inverted, Badge::Construct);
    }

    BracketExpressionTransition(int state, const Vector<SharedPtr<BracketItemMatcher>>& v, bool inverted, Badge)
        : RegexTransition(state), m_components(v), m_inverted(inverted) {}

    BracketExpressionTransition(int state, const BracketExpression& exp, int& out_error, Badge)
        : RegexTransition(state), m_inverted(exp.inverted) {
        const auto& items = exp.list;
        for (int i = 0; i < items.size(); i++) {
            const auto& exp = items[i];
            int error = 0;
            if (exp.type == BracketItem::Type::RangeExpression) {
                const auto& range = exp.expression.as<BracketRangeExpression>();
                auto matcher = make_shared<BracketRangeMatcher>(range.start, range.end, error);
                if (error != 0) {
                    out_error = error;
                    return;
                }
                m_components.add(move(matcher));
                continue;
            }

            const auto& single_expression = exp.expression.as<BracketSingleExpression>();
            SharedPtr<BracketItemMatcher> matcher;
            switch (single_expression.type) {
                case BracketSingleExpression::Type::SingleCollatingSymbol:
                case BracketSingleExpression::Type::GroupedCollatingSymbol:
                    matcher = make_shared<BracketSingleCollateMatcher>(single_expression.expression, error);
                    break;
                case BracketSingleExpression::Type::EquivalenceClass:
                    matcher = make_shared<BracketEquivalenceClassMatcher>(single_expression.expression, error);
                    break;
                case BracketSingleExpression::Type::CharacterClass:
                    matcher = make_shared<BracketCharacterClassMatcher>(single_expression.expression, error);
                    break;
            }

            if (error != 0) {
                out_error = error;
                return;
            }
            m_components.add(move(matcher));
        }
    }

private:
    Vector<SharedPtr<BracketItemMatcher>> m_components;
    bool m_inverted;
};

RegexGraph::RegexGraph(const ParsedRegex& regex_base, int cflags, int num_groups)
    : m_regex_base(regex_base), m_cflags(cflags), m_num_groups(num_groups) {}

bool RegexGraph::compile() {
    m_states.add(RegexState());
    RegexState* current_state = &m_states.last();

    auto add_forward_transition = [&]<typename Transition, typename... Args>(in_place_type_t<Transition>, Args... args) {
        current_state->transitions().add(make_shared<Transition>(m_states.size(), forward<Args>(args)...));
        m_states.add(RegexState());
        current_state = &m_states.last();
    };

    auto add_forward_transition_for_bracket_expression = [&](const BracketExpression& exp) -> int {
        auto result = BracketExpressionTransition::try_construct(m_states.size(), exp);
        if (result.is<int>()) {
            return result.as<int>();
        }
        current_state->transitions().add(move(result.as<SharedPtr<RegexTransition>>()));
        m_states.add(RegexState());
        current_state = &m_states.last();
        return 0;
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

    Function<bool(const ParsedRegex&, const Vector<int>&)> build_graph = [&](const ParsedRegex& regex,
                                                                             const Vector<int>& groups_to_match_base) {
        int alternative_state = m_states.size() - 1;
        Vector<int> end_states(regex.alternatives.size());
        for (int j = 0; j < regex.alternatives.size(); j++) {
            Vector<int> groups_to_match(groups_to_match_base);
            auto expression = regex.alternatives[j];
            if (j != 0) {
                m_states.add(RegexState());
                add_epsilon_transition(alternative_state, m_states.size() - 1);
                current_state = &m_states.last();
            }

            for (int i = 0; i < expression.parts.size(); i++) {
                int start_state = m_states.size() - 1;

                const SharedPtr<RegexSingleExpression>& exp = expression.parts[i];
                switch (exp->type) {
                    case RegexSingleExpression::Type::QuotedCharacter: {
                        assert(exp->expression.is<char>());
                        char ch = exp->expression.as<char>();
                        switch (ch) {
                            case 'd':
                            case 'D':
                                add_forward_transition(in_place_type<DigitCharacterTransition>, isupper(ch));
                                break;
                            case 'w':
                            case 'W':
                                add_forward_transition(in_place_type<WordCharacterTransition>, isupper(ch));
                                break;
                            case 's':
                            case 'S':
                                add_forward_transition(in_place_type<SpaceCharacterTransition>, isupper(ch));
                                break;
                            case 'b':
                            case 'B':
                                add_forward_transition(in_place_type<WordBoundaryTransition>, isupper(ch));
                                break;
                            case 'a':
                                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, '\a');
                                break;
                            case 'f':
                                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, '\f');
                                break;
                            case 'n':
                                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, '\n');
                                break;
                            case 'r':
                                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, '\r');
                                break;
                            case 't':
                                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, '\t');
                                break;
                            case 'v':
                                add_forward_transition(in_place_type<OrdinaryCharacterTransition>, '\v');
                                break;
                            default:
                                goto regex_single_expression_ordinary_character;
                        }
                        break;
                    }
                    case RegexSingleExpression::Type::OrdinaryCharacter:
                    regex_single_expression_ordinary_character:
                        assert(exp->expression.is<char>());
                        add_forward_transition(in_place_type<OrdinaryCharacterTransition>, exp->expression.as<char>());
                        break;
                    case RegexSingleExpression::Type::Any:
                        add_forward_transition(in_place_type<AnyCharacterTransition>);
                        break;
                    case RegexSingleExpression::Type::Group: {
                        const auto& sub_regex = exp->expression.as<ParsedRegex>();
                        groups_to_match.add(sub_regex.index);
                        add_forward_transition(in_place_type<BeginGroupCaptureTransition>, sub_regex.index);
                        if (!build_graph(sub_regex, groups_to_match)) {
                            return false;
                        }
                        add_forward_transition(in_place_type<EndGroupCaptureTransition>, exp->expression.as<ParsedRegex>().index);
                        break;
                    }
                    case RegexSingleExpression::Type::LeftAnchor:
                        add_forward_transition(in_place_type<LeftAnchorTransition>);
                        break;
                    case RegexSingleExpression::Type::RightAnchor:
                        add_forward_transition(in_place_type<RightAnchorTransition>);
                        break;
                    case RegexSingleExpression::Type::Backreference: {
                        assert(exp->expression.is<int>());
                        int group_index = exp->expression.as<int>();
                        if (!groups_to_match.includes(group_index)) {
                            m_error_code = REG_ESUBREG;
                            return false;
                        }
                        add_forward_transition(in_place_type<BackreferenceTransition>, group_index);
                        break;
                    }
                    case RegexSingleExpression::Type::BracketExpression: {
                        assert(exp->expression.is<BracketExpression>());
                        int ret = add_forward_transition_for_bracket_expression(exp->expression.as<BracketExpression>());
                        if (ret != 0) {
                            m_error_code = ret;
                            return false;
                        }
                        break;
                    }
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

            m_states.last().set_groups_to_match(move(groups_to_match));
            end_states.add(m_states.size() - 1);
        }

        if (end_states.size() > 1) {
            end_states.for_each([&](int from) {
                add_epsilon_transition(from, m_states.size());
            });
            m_states.add(RegexState());
            current_state = &m_states.last();
        }

        return true;
    };

    return build_graph(m_regex_base, Vector<int>());
}

Maybe<size_t> RegexGraph::try_match_at(const char* str, size_t index, int eflags, int state, Vector<regmatch_t>& dest_matches) const {
    const RegexState& current_state = m_states[state];
    if (current_state.transitions().empty()) {
        return { index };
    }

    for (int i = 0; i < current_state.transitions().size(); i++) {
        const auto& trans = current_state.transitions()[i];
        if (!trans->try_transition(str, index, eflags | m_cflags, m_states, state, index, dest_matches)) {
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
