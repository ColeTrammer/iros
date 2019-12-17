#pragma once

#include "state_table.h"

class Generator {
public:
    Generator(const StateTable& table, const Vector<StringView>& identifiers, const Vector<StringView>& token_types,
              const String& output_name);
    ~Generator();

    void generate_token_type_header(const String& path);

private:
    const StateTable& m_table;
    const Vector<StringView>& m_identifiers;
    const Vector<StringView>& m_token_types;
    String m_output_name;
};