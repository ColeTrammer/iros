#pragma once

#include "state_table.h"

class Generator {
public:
    Generator(const StateTable& table, const Vector<StringView>& identifiers, const Vector<StringView>& token_types,
              const LinkedList<String>& literals, const String& output_name, bool dont_overwrite, const String& name_space);
    ~Generator();

    void generate_token_type_header(const String& path);
    void generate_generic_parser(String path);
    void generate_value_header(const String& path, const String& value_types_header);

private:
    const StateTable& m_table;
    const Vector<StringView>& m_identifiers;
    const Vector<StringView>& m_token_types;
    const LinkedList<String>& m_literals;
    String m_output_name;
    String m_name_space;
    bool m_dont_overwrite { false };
};
