#pragma once

#include "extended_grammar.h"

class StateTable {
public:
    StateTable(const ExtendedGrammar& grammar);
    ~StateTable();

    const ExtendedGrammar& grammar() const { return m_grammar; }

    String stringify();

private:
    const ExtendedGrammar& m_grammar;
};