#include "state_table.h"

StateTable::StateTable(const ExtendedGrammar& grammar) : m_grammar(grammar) {}

StateTable::~StateTable() {}

String StateTable::stringify() {
    return "State Table";
}