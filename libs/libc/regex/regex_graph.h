#pragma once

#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/vector.h>

#include "regex_value.h"

class RegexTransition {
public:
    RegexTransition(int state) : m_state(state) {}
    virtual ~RegexTransition() {}

    virtual bool can_transition(const char* s, size_t index, int flags) const = 0;
    size_t transition(int& state) const {
        state = m_state;
        return num_characters_matched();
    };

    virtual size_t num_characters_matched() const { return 1; }

    virtual void dump() const = 0;

protected:
    int state() const { return m_state; }

private:
    int m_state {};
};

class RegexState {
public:
    Vector<SharedPtr<RegexTransition>>& transitions() { return m_transitions; }
    const Vector<SharedPtr<RegexTransition>>& transitions() const { return m_transitions; }

private:
    Vector<SharedPtr<RegexTransition>> m_transitions;
};

class RegexGraph {
public:
    RegexGraph(const ParsedRegex& regex, int cflags);

    void dump() const;

    Vector<regmatch_t> do_match(const char* str, int eflags) const;

private:
    Maybe<size_t> try_match_at(const char* s, size_t index, int cflags, int state, Vector<regmatch_t>& dest_matches) const;

    Vector<RegexState> m_states;
    int m_cflags;
};