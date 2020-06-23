#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "generator.h"
#include "literal.h"

Generator::Generator(const StateTable& table, const Vector<StringView>& identifiers, const Vector<StringView>& token_types,
                     const LinkedList<String>& literals, const String& output_name, bool dont_overwite, bool has_value_header,
                     const String& name_space)
    : m_table(table)
    , m_identifiers(identifiers)
    , m_token_types(token_types)
    , m_literals(literals)
    , m_output_name(output_name)
    , m_name_space(name_space)
    , m_dont_overwrite(dont_overwite)
    , m_has_value_header(has_value_header) {}

Generator::~Generator() {}

void Generator::generate_token_type_header(const String& path) {
    fprintf(stderr, "Writing token types to %s.\n", path.string());

    int ofd = open(path.string(), O_CREAT | O_WRONLY | O_TRUNC | (m_dont_overwrite ? O_EXCL : 0), 0666);
    if (ofd == -1) {
        perror("opening generic parser");
        if (errno == EEXIST) {
            exit(2);
        }
        exit(1);
    }

    FILE* token_type_header = fdopen(ofd, "w");
    fprintf(token_type_header, "#pragma once\n\n");

    fprintf(token_type_header, "enum class %sTokenType {\n", String(m_output_name).to_title_case().string());
    m_identifiers.for_each([&](const auto& id) {
        String name = String(id);
        fprintf(token_type_header, "    %s,\n", name.string());
    });
    fprintf(token_type_header, "};\n");
    if (fclose(token_type_header) != 0) {
        perror("fclose");
        exit(1);
    }
}

void Generator::generate_generic_parser(String path) {
    struct ReductionInfo {
        String function_name;
        Vector<StringView> args;
        int number;
        StringView name;
    };

    auto component_name_to_type = [&](const StringView& view) -> String {
        if (m_token_types.includes(view)) {
            return String::format("%sTerminal", String(m_output_name).to_title_case().string());
        }
        return String(view).to_title_case();
    };

    HashMap<int, Vector<ReductionInfo>> reduce_info;
    m_table.rules().for_each([&](const Vector<FinalRule>& rules) {
        rules.for_each([&](const FinalRule& rule) {
            if (rule.original_number() == 0) {
                return;
            }

            ReductionInfo info { "reduce_", rule.components(), rule.original_number(), rule.lhs() };
            info.function_name += String(rule.lhs()).to_lower_case();
            for (int i = 0; i < rule.components().size(); i++) {
                auto& v = rule.components().get(i);
                info.function_name += String::format("%c%s", i == 0 ? '$' : '_', String(v).to_lower_case().string());
            }

            if (!reduce_info.get(rule.original_number())) {
                reduce_info.put(rule.original_number(), Vector<ReductionInfo>());
            }

            reduce_info.get(rule.original_number())->add(info);
        });
    });

    fprintf(stderr, "Writing generic parser to %s.\n", path.string());

    int ofd = open(path.string(), O_CREAT | O_WRONLY | O_TRUNC | (m_dont_overwrite ? O_EXCL : 0), 0666);
    if (ofd == -1) {
        perror("opening generic parser");
        if (errno == EEXIST) {
            exit(2);
        }
        exit(1);
    }

    FILE* file = fdopen(ofd, "w");
    fprintf(file, "#pragma once\n\n");

    fprintf(file, "#include <assert.h>\n");
    fprintf(file, "#include <stdio.h>\n\n");
    fprintf(file, "#include <parser/generic_parser.h>\n");
    fprintf(file, "#include <parser/generic_token.h>\n");
    fprintf(file, "%s", String::format("#include \"%s_token_type.h\"\n", m_output_name.to_lower_case().string()).string());

    String value_string = String::format("%sValue", String(m_output_name).to_title_case().string());
    if (m_has_value_header) {
        fprintf(file, "#include \"%s_value.h\"\n", m_output_name.string());
    }

    if (!m_name_space.is_empty()) {
        fprintf(file, "\nnamespace %s {\n", m_name_space.string());
    }

    if (!m_has_value_header) {
        fprintf(file, "\ntemplate<typename %s>", value_string.string());
    }

    fprintf(file, "\nclass Generic%sParser : public GenericParser<%sTokenType, %s> {\n", m_output_name.to_title_case().string(),
            m_output_name.string(), value_string.string());
    fprintf(file, "public:\n");
    fprintf(file, "    using Token = GenericToken<%sTokenType, %s>;\n\n", m_output_name.string(), value_string.string());
    fprintf(file, "    Generic%sParser(GenericLexer<%sTokenType, %s>& lexer) : GenericParser<%sTokenType, %s>(lexer) {}\n",
            m_output_name.string(), m_output_name.string(), value_string.string(), m_output_name.string(), value_string.string());
    fprintf(file, "    virtual ~Generic%sParser() override {}\n\n", m_output_name.string());

    fprintf(file, "    static const char* token_type_to_string(%sTokenType type) {\n",
            String(m_output_name.to_title_case().string()).string());

    fprintf(file, "        switch (type) {\n");

    m_identifiers.for_each([&](const auto& id) {
        String name = m_literals.includes(String(id)) ? String(token_to_literal(id)).string() : String(id);
        fprintf(file, "            case %sTokenType::%s:\n", String(m_output_name).to_title_case().string(), String(id).string());
        fprintf(file, "                return \"%s\";\n", name.string());
    });

    fprintf(file, "            default:\n");
    fprintf(file, "                return \"Invalid token\";\n");
    fprintf(file, "        }\n");
    fprintf(file, "    }\n\n");

    fprintf(file, "    static %sTokenType text_to_token_type(const StringView& text) {\n", String(m_output_name).to_title_case().string());

    m_token_types.for_each([&](const auto& t) {
        if (m_literals.includes(String(t))) {
            fprintf(file, "        if (text == \"%s\")\n", String(token_to_literal(t)).string());
            fprintf(file, "            return %sTokenType::%s;\n", String(m_output_name).to_title_case().string(), String(t).string());
        }
    });

    fprintf(file, "        return %sTokenType::End;\n", String(m_output_name).to_title_case().string());
    fprintf(file, "    }\n\n");

    if (m_has_value_header) {
        auto result_type = component_name_to_type(reduce_info.get(1)->first().name);
        fprintf(file, "    const %s& result() const {\n", result_type.string());
        fprintf(file, "        return this->peek_value_stack().as<%s>();\n", result_type.string());
        fprintf(file, "    }\n\n");
    }

    fprintf(file, "    virtual bool is_valid_token_type_in_current_state_for_shift(%sTokenType type) const override;\n",
            String(m_output_name).to_title_case().string());
    fprintf(file, "    virtual bool is_valid_token_type_in_current_state(%sTokenType type) const override;\n",
            String(m_output_name).to_title_case().string());
    fprintf(file, "    virtual bool parse() override;\n\n");
    {
        path.remove_index(path.size() - 1);
        path.remove_index(path.size() - 1);
        path += "_impl.cpp";
        int ofd = open(path.string(), O_CREAT | O_WRONLY | O_TRUNC | (m_dont_overwrite ? O_EXCL : 0), 0644);
        if (ofd == -1) {
            perror("opening generic parser");
            if (errno == EEXIST) {
                exit(2);
            }
            exit(1);
        }
        FILE* file = fdopen(ofd, "w");
        fprintf(file, "#include \"generic_%s_parser.h\"\n\n", String(m_output_name).to_lower_case().string());

        if (!m_name_space.is_empty()) {
            fprintf(file, "namespace %s {\n\n", m_name_space.string());
        }

        if (!m_has_value_header) {
            fprintf(file,
                    "template<typename %s> bool Generic%sParser<%s>::is_valid_token_type_in_current_state_for_shift(%sTokenType type) "
                    "const {\n",
                    value_string.string(), String(m_output_name).to_title_case().string(), value_string.string(),
                    String(m_output_name).to_title_case().string());
        } else {
            fprintf(file,
                    "bool Generic%sParser::is_valid_token_type_in_current_state_for_shift(%sTokenType type) "
                    "const {\n",
                    String(m_output_name).to_title_case().string(), String(m_output_name).to_title_case().string());
        }
        fprintf(file, "    switch (this->current_state()) {\n");
        for (int i = 0; i < m_table.table().size(); i++) {
            const auto& row = m_table.table()[i];
            fprintf(file, "        case %d:\n", i);
            fprintf(file, "            switch (type) {\n");

            bool did_something = false;
            row.for_each_key([&](const auto& s) {
                if (row.get(s)->type == Action::Type::Shift) {
                    fprintf(file, "                case %sTokenType::%s:\n", String(m_output_name).to_title_case().string(),
                            String(s).string());
                    did_something = true;
                }
            });
            if (did_something) {
                fprintf(file, "                    return true;\n");
            }
            fprintf(file, "                default:\n");
            fprintf(file, "                    return false;\n");
            fprintf(file, "            }\n");
        }
        fprintf(file, "        default:\n");
        fprintf(file, "            return false;\n");
        fprintf(file, "    }\n");
        fprintf(file, "}\n\n");

        if (!m_has_value_header) {
            fprintf(file,
                    "template<typename %s> bool Generic%sParser<%s>::is_valid_token_type_in_current_state(%sTokenType type) const {\n",
                    value_string.string(), String(m_output_name).to_title_case().string(), value_string.string(),
                    String(m_output_name).to_title_case().string());
        } else {
            fprintf(file, "bool Generic%sParser::is_valid_token_type_in_current_state(%sTokenType type) const {\n",
                    String(m_output_name).to_title_case().string(), String(m_output_name).to_title_case().string());
        }
        fprintf(file, "    switch (this->current_state()) {\n");
        for (int i = 0; i < m_table.table().size(); i++) {
            const auto& row = m_table.table()[i];
            fprintf(file, "        case %d:\n", i);
            fprintf(file, "            switch (type) {\n");
            row.for_each_key([&](const auto& s) {
                fprintf(file, "                case %sTokenType::%s:\n", String(m_output_name).to_title_case().string(),
                        String(s).string());
            });
            fprintf(file, "                    return true;\n");
            fprintf(file, "                default:\n");
            fprintf(file, "                    return false;\n");
            fprintf(file, "            }\n");
        }
        fprintf(file, "        default:\n");
        fprintf(file, "            return false;\n");
        fprintf(file, "    }\n");
        fprintf(file, "}\n\n");

        if (!m_has_value_header) {
            fprintf(file, "template<typename %s> bool Generic%sParser<%s>::parse() {\n", value_string.string(),
                    String(m_output_name).to_title_case().string(), value_string.string());
        } else {
            fprintf(file, "bool Generic%sParser::parse() {\n", String(m_output_name).to_title_case().string());
        }
        fprintf(file, "    for (; !this->error() ;) {\n");
        fprintf(file, "        switch (this->current_state()) {\n");

        for (int i = 0; i < m_table.table().size(); i++) {
            const HashMap<StringView, Action>& row = m_table.table()[i];
            fprintf(file, "%s", String::format("            case %d:\n", i).string());
            fprintf(file, "                switch (this->peek_token_type()) {\n");

            HashMap<Action, Vector<StringView>> actions;
            row.for_each_key([&](const StringView& name) {
                const Action& action = *row.get(name);
                if (!actions.get(action)) {
                    actions.put(action, Vector<StringView>());
                }
                actions.get(action)->add(name);
            });

            actions.for_each_key([&](const Action& action) {
                const auto& names = *actions.get(action);
                for (int i = 0; i < names.size(); i++) {
                    fprintf(file, "                    case %sTokenType::%s:%s\n", m_output_name.string(), String(names[i]).string(),
                            i == names.size() - 1 ? " {" : "");
                }

                switch (action.type) {
                    case Action::Type::Accept:
                        fprintf(file, "                        return true;\n");
                        break;
                    case Action::Type::Jump:
                        fprintf(file, "                        this->jump_to(%d);\n", action.number);
                        fprintf(file, "                        continue;\n");
                        break;
                    case Action::Type::Shift:
                        fprintf(file, "                        this->consume_token();\n");
                        fprintf(file, "                        this->push_state_stack(%d);\n", action.number);
                        fprintf(file, "                        continue;\n");
                        break;
                    case Action::Type::Reduce: {
                        const ReductionInfo* info = reduce_info.get(action.number)->first_match([&](const ReductionInfo& t) {
                            return t.number == action.number;
                        });
                        assert(info);
                        String args = "";
                        for (int i = 0; i < info->args.size(); i++) {
                            fprintf(file, "                        %s v%d = this->pop_stack_state();\n", value_string.string(),
                                    info->args.size() - i - 1);
                            if (!m_has_value_header) {
                                args += String::format("v%d", i);
                            } else {
                                args += String::format("v%d.as<%s>()", i, component_name_to_type(info->args[i]).string());
                            }
                            if (i != info->args.size() - 1) {
                                args += ", ";
                            }
                        }
                        fprintf(file, "                        this->push_value_stack(%s(%s));\n", info->function_name.string(),
                                args.string());
                        fprintf(file, "                        this->reduce(%sTokenType::%s);\n", m_output_name.string(),
                                String(info->name).string());
                        fprintf(file, "                        continue;\n");
                        break;
                    }
                }

                fprintf(file, "                    }\n");
            });

            fprintf(file, "                    default:\n");
            fprintf(file, "                        on_error(this->peek_token_type());\n");
            fprintf(file, "                        return false;\n");

            fprintf(file, "                }\n");
        }

        fprintf(file, "        }\n");
        fprintf(file, "    }\n\n");
        fprintf(file, "    return false;\n");
        fprintf(file, "}\n");

        if (!m_name_space.is_empty()) {
            fprintf(file, "\n}\n");
        }
    }

    fprintf(file, "protected:\n");

    HashMap<String, bool> already_declared;

    reduce_info.for_each([&](const Vector<ReductionInfo>& infos) {
        infos.for_each([&](const ReductionInfo& info) {
            if (already_declared.get(info.function_name)) {
                return;
            }

            String arg_list = "(";
            for (int i = 0; i < info.args.size(); i++) {
                String type = m_has_value_header ? component_name_to_type(info.args[i]) : value_string;
                arg_list += String::format("%s&", type.string());

                if (i != info.args.size() - 1) {
                    arg_list += ", ";
                }
            }
            arg_list += ")";

            String return_type = m_has_value_header ? component_name_to_type(info.name) : value_string;

            fprintf(file, "    virtual %s %s%s = 0;\n", return_type.string(), info.function_name.string(), arg_list.string());
            already_declared.put(info.function_name, true);
        });
    });

    fprintf(file,
            "\n    virtual void on_error(%sTokenType type) { fprintf(stderr, \"Unexpected token: %%s (state %%d)\\n\", "
            "token_type_to_string(type), this->current_state()); }\n",
            m_output_name.string());

    fprintf(file, "};\n");
    if (!m_name_space.is_empty()) {
        fprintf(file, "\n}\n");
    }

    if (fclose(file) != 0) {
        perror("close");
        exit(1);
    }
}

void Generator::generate_value_header(const String& path, const String& value_types_header) {
    fprintf(stderr, "Writing value header to %s.\n", path.string());

    int ofd = open(path.string(), O_CREAT | O_WRONLY | O_TRUNC | (m_dont_overwrite ? O_EXCL : 0), 0666);
    if (ofd == -1) {
        perror("opening generic parser");
        if (errno == EEXIST) {
            exit(2);
        }
        exit(1);
    }

    FILE* file = fdopen(ofd, "w");

    fprintf(file, "#pragma once\n\n");
    fprintf(file, "#include <liim/variant.h>\n");
    fprintf(file, "#include %s\n\n", value_types_header.string());

    if (!m_name_space.is_empty()) {
        fprintf(file, "namespace %s {\n\n", m_name_space.string());
    }

    fprintf(file, "using %sValue = LIIM::Variant<\n    %sTerminal", m_output_name.string(), m_output_name.string());
    for (const auto& id_view : m_identifiers) {
        String id(id_view);
        if (!m_token_types.includes(id_view) && id_view != "End" && id_view != "__start") {
            fprintf(file, ",\n    %s", id.to_title_case().string());
        }
    }
    fprintf(file, "\n>;\n");

    if (!m_name_space.is_empty()) {
        fprintf(file, "\n}\n");
    }

    if (fclose(file) != 0) {
        perror("fclose");
        exit(1);
    }
}
