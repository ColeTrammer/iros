#pragma once

#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/vector.h>

#include "regex_value.h"

class RegexTransition {
public:
    RegexTransition(int state) : m_state(state) {}
    virtual ~RegexTransition() {}

    bool try_transition(const char* s, size_t index, int flags, int& state, size_t& dest_index, Vector<regmatch_t>& dest_matches) const {
        auto result = do_try_transition(s, index, flags, dest_matches);
        if (result.has_value()) {
            state = m_state;
            dest_index += result.value();
            return true;
        }

        return false;
    }

    virtual void dump() const = 0;

    virtual SharedPtr<RegexTransition> clone_with_shift(int shift) const = 0;

protected:
    int state() const { return m_state; }

    virtual size_t num_characters_matched() const { return 1; }

    virtual Maybe<size_t> do_try_transition(const char* s, size_t index, int flags, Vector<regmatch_t>& dest_matches) const = 0;

private:
    int m_state {};
};

class RegexState {
public:
    Vector<SharedPtr<RegexTransition>>& transitions() { return m_transitions; }
    const Vector<SharedPtr<RegexTransition>>& transitions() const { return m_transitions; }

    static RegexState copy_with_shift(const RegexState& other, int shift) {
        RegexState ret;
        other.transitions().for_each([&](const auto& tr) {
            ret.transitions().add(tr->clone_with_shift(shift));
        });
        return ret;
    }

private:
    Vector<SharedPtr<RegexTransition>> m_transitions;
};

class RegexGraph {
public:
    RegexGraph(const ParsedRegex& regex, int cflags, int num_groups);

    void dump() const;

    Maybe<Vector<regmatch_t>> do_match(const char* str, int eflags) const;

private:
    Maybe<size_t> try_match_at(const char* s, size_t index, int cflags, int state, Vector<regmatch_t>& dest_matches) const;

    Vector<RegexState> m_states;
    int m_cflags;
    int m_num_groups;
};