#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "generator.h"

Generator::Generator(const StateTable& table, const Vector<StringView>& identifiers, const Vector<StringView>& token_types,
                     const String& output_name)
    : m_table(table), m_identifiers(identifiers), m_token_types(token_types), m_output_name(output_name) {}

Generator::~Generator() {}

void Generator::generate_token_type_header(const String& path) {
    fprintf(stderr, "Writing token types to %s.\n", path.string());

    int ofd = open(path.string(), O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (ofd == -1) {
        perror("opening output header");
        exit(1);
    }

    FILE* token_type_header = fdopen(ofd, "w");
    fprintf(token_type_header, "#pragma once\n\n");

    fprintf(token_type_header, "enum class %sTokenType {\n", m_output_name.to_title_case().string());
    m_identifiers.for_each([&](const auto& id) {
        String name = String(id);
        fprintf(token_type_header, "%s, ", name.string());
    });
    fprintf(token_type_header, "};\n");

    if (fclose(token_type_header) != 0) {
        perror("fclose");
        exit(1);
    }
}