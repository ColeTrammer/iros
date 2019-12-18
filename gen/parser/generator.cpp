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
        fprintf(token_type_header, "    %s,\n", name.string());
    });
    fprintf(token_type_header, "};\n\n");

    fprintf(token_type_header, "constexpr const char* %s_token_type_to_string(%sTokenType type) {\n",
            m_output_name.to_lower_case().string(), String(m_output_name.to_title_case().string()).string());

    fprintf(token_type_header, "    switch (type) {\n");

    m_identifiers.for_each([&](const auto& id) {
        fprintf(token_type_header, "        case %sTokenType::%s:\n", m_output_name.to_title_case().string(), String(id).string());
        fprintf(token_type_header, "            return \"%s\";\n", String(id).string());
    });

    fprintf(token_type_header, "        default:\n");
    fprintf(token_type_header, "            return \"Invalid token\";\n");
    fprintf(token_type_header, "    }\n");

    fprintf(token_type_header, "}\n");
    if (fclose(token_type_header) != 0) {
        perror("fclose");
        exit(1);
    }
}

void Generator::generate_generic_parser(const String& path) {
    struct ReductionInfo {
        String function_name;
        int arg_count;
        StringView name;
    };

    HashMap<int, ReductionInfo> reduce_info;
    m_table.rules().for_each([&](const FinalRule& rule) {
        if (rule.original_number() == 0) {
            return;
        }

        ReductionInfo info { "reduce_", rule.components().size(), rule.lhs() };
        info.function_name += String(rule.lhs()).to_lower_case();
        rule.components().for_each([&](const auto& v) {
            info.function_name += String::format("_%s", String(v).to_lower_case().string());
        });

        reduce_info.put(rule.original_number(), info);
    });

    fprintf(stderr, "Writing generic parser to %s.\n", path.string());

    int ofd = open(path.string(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (ofd == -1) {
        perror("opening generic parser");
        exit(1);
    }

    FILE* file = fdopen(ofd, "w");
    fprintf(file, "#pragma once\n\n");

    fprintf(file, "#include <assert.h>\n");
    fprintf(file, "#include <stdio.h>\n\n");
    fprintf(file, "#include \"generic_parser.h\"\n");
    fprintf(file, "#include \"generic_token.h\"\n");
    fprintf(file, "%s", String::format("#include \"%s_token_type.h\"\n\n", m_output_name.to_lower_case().string()).string());

    fprintf(file, "%s",
            String::format("template<typename Value> class Generic%sParser : public GenericParser<%sTokenType, Value> {\n",
                           m_output_name.to_title_case().string(), m_output_name.string())
                .string());
    fprintf(file, "public:\n");
    fprintf(file, "%s", String::format("    using Token = GenericToken<%sTokenType, Value>;\n\n", m_output_name.string()).string());
    fprintf(file, "%s",
            String::format("    Generic%sParser(const Vector<Token>& tokens) : GenericParser<%sTokenType, Value>(tokens) {}\n",
                           m_output_name.string(), m_output_name.string())
                .string());
    fprintf(file, "%s", String::format("    ~Generic%sParser() override {}\n\n", m_output_name.string()).string());
    fprintf(file, "    virtual bool parse() override {\n");
    fprintf(file, "        for (;;) {\n");
    fprintf(file, "            switch (this->current_state()) {\n");

    for (int i = 0; i < m_table.table().size(); i++) {
        const HashMap<StringView, Action>& row = m_table.table()[i];
        fprintf(file, "%s", String::format("                case %d:\n", i).string());
        fprintf(file, "                    switch (this->peek_token_type()) {\n");

        bool default_used = false;
        row.for_each_key([&](const StringView& name) {
            const Action& action = *row.get(name);

            if (name == "End" && action.type == Action::Type::Reduce) {
                fprintf(file, "                        default: {\n");
                default_used = true;
            } else {
                fprintf(file, "                        case %sTokenType::%s: {\n", m_output_name.string(), String(name).string());
            }

            switch (action.type) {
                case Action::Type::Accept:
                    fprintf(file, "                            return true;\n");
                    break;
                case Action::Type::Jump:
                    fprintf(file, "                            this->jump_to(%d);\n", action.number);
                    fprintf(file, "                            continue;\n");
                    break;
                case Action::Type::Shift:
                    fprintf(file, "                            this->consume_token();\n");
                    fprintf(file, "                            this->push_state_stack(%d);\n", action.number);
                    fprintf(file, "                            continue;\n");
                    break;
                case Action::Type::Reduce: {
                    const ReductionInfo& info = *reduce_info.get(action.number);
                    String args = "";
                    for (int i = 0; i < info.arg_count; i++) {
                        fprintf(file, "                            int v%d = this->pop_stack_state();\n", i);
                        args += String::format("v%d", i);
                        if (i != info.arg_count - 1) {
                            args += ", ";
                        }
                    }
                    fprintf(file, "                            this->push_value_stack(%s(%s));\n", info.function_name.string(),
                            args.string());

                    fprintf(file, "                            this->reduce(%sTokenType::%s);\n", m_output_name.string(),
                            String(info.name).string());

                    fprintf(file, "                            continue;\n");
                    break;
                }
            }

            fprintf(file, "                        }\n");
        });

        if (!default_used) {
            fprintf(file, "                        default:\n");
            fprintf(file, "                            on_error(this->peek_token_type());\n");
            fprintf(file, "                            return false;\n");
        }

        fprintf(file, "                    }\n");
    }

    fprintf(file, "            }\n");
    fprintf(file, "        }\n\n");
    fprintf(file, "        assert(false);\n");
    fprintf(file, "        return false;\n");
    fprintf(file, "    }\n\n");
    fprintf(file, "protected:\n");

    HashMap<String, bool> already_declared;

    reduce_info.for_each([&](const ReductionInfo& info) {
        if (already_declared.get(info.function_name)) {
            return;
        }

        String arg_list = "(";
        for (int i = 0; i < info.arg_count; i++) {
            if (i == 0) {
                arg_list += "Value v";
            } else {
                arg_list += "Value";
            }

            if (i != info.arg_count - 1) {
                arg_list += ", ";
            }
        }
        arg_list += ")";

        String return_string = info.arg_count == 0 ? "Value()" : "v";

        fprintf(file, "    virtual Value %s%s {\n", info.function_name.string(), arg_list.string());
        fprintf(file, "#ifdef GENERIC_%s_PARSER_DEBUG\n", String(m_output_name).to_upper_case().string());
        fprintf(file, "        fprintf(stderr, \"%%s called.\\n\", __FUNCTION__);\n");
        fprintf(file, "#endif /* GENERIC_%s_PARSER_DEBUG */\n", String(m_output_name).to_upper_case().string());
        fprintf(file, "        return %s;\n", return_string.string());
        fprintf(file, "    }\n");

        already_declared.put(info.function_name, true);
    });

    fprintf(file,
            "\n    virtual void on_error(%sTokenType type) { fprintf(stderr, \"Unexpected token: %%s (state %%d)\\n\", "
            "%s_token_type_to_string(type), this->current_state()); }\n",
            m_output_name.to_title_case().string(), String(m_output_name.to_lower_case()).string());

    fprintf(file, "};\n");
    if (fclose(file) != 0) {
        perror("close");
        exit(1);
    }
}