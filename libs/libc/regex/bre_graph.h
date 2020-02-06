#pragma once

#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/vector.h>

#include "bre_value.h"

class BRETransition {
public:
    BRETransition(int state) : m_state(state) {}
    virtual ~BRETransition() {}

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

class BREState {
public:
    Vector<SharedPtr<BRETransition>>& transitions() { return m_transitions; }
    const Vector<SharedPtr<BRETransition>>& transitions() const { return m_transitions; }

private:
    Vector<SharedPtr<BRETransition>> m_transitions;
};

class BREGraph {
public:
    BREGraph(const BRE& bre, int cflags);

    void dump() const;

private:
    Vector<BREState> m_states;
    int m_cflags;
};