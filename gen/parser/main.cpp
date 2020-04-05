#include <assert.h>
#include <fcntl.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "extended_grammar.h"
#include "generator.h"
#include "item_set.h"
#include "lexer.h"
#include "literal.h"
#include "rule.h"
#include "state_table.h"

void print_usage_and_exit(char** argv) {
    fprintf(stderr, "Usage: %s <grammar>\n", argv[0]);
    exit(1);
}

static StringView reduce_grouping(const Vector<Token<TokenType>>& tokens, Vector<StringView>& token_types, Vector<Rule>& rules,
                                  LinkedList<String>& literals, int position, int& end_position, int& rule_index) {
    static LinkedList<String> created_strings;

    Vector<Vector<StringView>> result;
    result.add(Vector<StringView>());
    for (end_position = position; end_position < tokens.size() && tokens[end_position].type() != TokenType::TokenRightParenthesis;
         end_position++) {
        auto& token = tokens.get(end_position);
        switch (token.type()) {
            case TokenType::TokenWord:
                result.last().add(token.text());
                break;
            case TokenType::TokenLeftParenthesis:
                result.last().add(reduce_grouping(tokens, token_types, rules, literals, end_position + 1, end_position, rule_index));
                break;
            case TokenType::TokenLiteral: {
                String real_name = literal_to_token(token.text());
                literals.add(real_name);
                StringView view = StringView(&literals.tail()[0], &literals.tail()[literals.tail().size() - 1]);
                if (!token_types.includes(view)) {
                    token_types.add(view);
                }
                result.last().add(view);
                break;
            }
            case TokenType::TokenPipe:
                result.add(Vector<StringView>());
                break;
            default:
                fprintf(stderr, "Syntax error\n");
                exit(1);
                break;
        }
    }

    String prefix = "match_";
    if (++end_position < tokens.size()) {
        switch (tokens[end_position].type()) {
            case TokenType::TokenQuestionMark:
                prefix = "optional_";
                break;
            case TokenType::TokenStar:
                prefix = "zero_or_more_";
                break;
            case TokenType::TokenPlus:
                prefix = "one_or_more_";
                break;
            default:
                break;
        }
    }

    for (int i = 0; i < result.size(); i++) {
        const auto& list = result.get(i);
        for (int j = 0; j < list.size(); j++) {
            prefix += String(list.get(j));
            if (j != list.size() - 1) {
                prefix += "_";
            }
        }

        if (i != result.size() - 1) {
            prefix += "_or_";
        }
    }

    created_strings.add(prefix.to_lower_case());
    StringView view = StringView(&created_strings.tail()[0], &created_strings.tail()[created_strings.tail().size() - 1]);
    if (end_position < tokens.size()) {
        switch (tokens[end_position].type()) {
            case TokenType::TokenStar: {
                Rule e;
                e.name() = view;
                if (!rules.includes(e)) {
                    e.set_number(rule_index++);
                    rules.add(e);
                }
            }
                // Fall through
            case TokenType::TokenPlus: {
                for (int i = 0; i < result.size(); i++) {
                    Rule r;
                    r.name() = view;
                    Vector<StringView> rhs(result[i]);
                    rhs.insert(view, 0);
                    r.components() = rhs;
                    if (!rules.includes(r)) {
                        r.set_number(rule_index++);
                        rules.add(r);
                    }
                }
                goto regular;
            }
            case TokenType::TokenQuestionMark: {
                Rule e;
                e.name() = view;
                if (!rules.includes(e)) {
                    e.set_number(rule_index++);
                    rules.add(e);
                }
                goto regular;
            }
            default:
                end_position--;
                goto regular;
        }
    } else {
    regular:
        for (int i = 0; i < result.size(); i++) {
            Rule r;
            r.name() = view;
            r.components() = result[i];
            if (!rules.includes(r)) {
                r.set_number(rule_index++);
                rules.add(r);
            }
        }
    }

    return view;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage_and_exit(argv);
    }

    struct stat info;
    if (stat(argv[1], &info) != 0) {
        perror("stat");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    char* contents = reinterpret_cast<char*>(mmap(nullptr, info.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    if (contents == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (close(fd) != 0) {
        perror("close");
        return 1;
    }

    Lexer lexer(contents, info.st_size);
    auto tokens = lexer.lex();

    StringView* start_name = nullptr;

    Vector<StringView> token_types;
    LinkedList<String> literals;

    bool out_of_declarations = false;
    for (int i = 0; i < tokens.size(); i++) {
        auto& token = tokens[i];
        switch (token.type()) {
            case TokenType::TokenWord:
                if (!out_of_declarations) {
                    token_types.add(token.text());
                }
                break;
            case TokenType::TokenTokenMarker:
                continue;
            case TokenType::TokenStartMarker:
                start_name = &tokens[++i].text();
                break;
            case TokenType::TokenLiteral: {
                String real_name = literal_to_token(token.text());
                literals.add(real_name);
                StringView view = StringView(&literals.tail()[0], &literals.tail()[literals.tail().size() - 1]);
                if (!token_types.includes(view))
                    token_types.add(view);
                break;
            }
            case TokenType::TokenPercentPercent:
                out_of_declarations = true;
                break;
            default:
                continue;
        }
    }

    Vector<Rule> rules;

    bool start = false;
    StringView* rule_name = nullptr;
    Rule rule;
    int num = 1;

    for (int i = 0; i < tokens.size(); i++) {
        auto& token = tokens.get(i);
        if (token.type() == TokenType::TokenPercentPercent) {
            start = true;
            continue;
        }

        if (start) {
            switch (token.type()) {
                case TokenType::TokenWord:
                    rule.components().add(token.text());
                    break;
                case TokenType::TokenColon:
                    assert(rule_name);
                    rule.name() = *rule_name;
                    break;
                case TokenType::TokenSemicolon:
                    break;
                case TokenType::TokenLhs:
                    if (!rule_name) {
                        rule_name = &token.text();
                        if (!start_name) {
                            start_name = rule_name;
                        }
                        break;
                    }
                    rule_name = &token.text();
                    // Fall through
                case TokenType::TokenEnd:
                case TokenType::TokenPipe:
                    if (!rules.includes(rule)) {
                        rule.set_number(num++);
                        rules.add(rule);
                    }
                    rule.components().clear();
                    break;
                case TokenType::TokenLiteral: {
                    const String* lit = nullptr;
                    literals.for_each([&](const auto& s) {
                        if (s == literal_to_token(token.text())) {
                            lit = &s;
                        }
                    });
                    assert(lit);
                    rule.components().add(StringView(&(*lit)[0], &(*lit)[lit->size() - 1]));
                    break;
                }
                case TokenType::TokenLeftParenthesis: {
                    StringView added = reduce_grouping(tokens, token_types, rules, literals, i + 1, i, num);
                    rule.components().add(added);
                    break;
                }
                default:
                    assert(false);
            }
        }
    }

    if (rules.size() == 0) {
        fprintf(stderr, "No rules.\n");
        exit(1);
    }

    rules.for_each([&](auto& rule) {
        fprintf(stderr, "%s\n", rule.stringify().string());
    });
    fprintf(stderr, "\n");

    Vector<StringView> identifiers;
    identifiers.add("End");
    token_types.for_each([&](auto& s) {
        identifiers.add(s);
    });

    rules.for_each([&](auto& rule) {
        if (!identifiers.includes(rule.name())) {
            identifiers.add(rule.name());
        }
    });

    Rule dummy_start;
    dummy_start.name() = "__start";
    dummy_start.components().add(*start_name);
    dummy_start.set_number(0);

    rules.insert(dummy_start, 0);
    Rule& start_rule = rules.get(0);

    identifiers.add("__start");
    fprintf(stderr, "Added __start rule: %s\n", dummy_start.stringify().string());

    auto sets = ItemSet::create_item_sets(start_rule, rules, token_types);
    sets.for_each([&](auto& set) {
        fprintf(stderr, "%s\n", set->stringify().string());
    });

    ExtendedGrammar extended_grammar(sets, token_types);
    fprintf(stderr, "\n%s\n", extended_grammar.stringify().string());

    StateTable state_table(extended_grammar, identifiers, token_types);
    fprintf(stderr, "%s\n", state_table.stringify().string());

    *strrchr(argv[1], '.') = '\0';

    String output_name = argv[1];
    String prepend = "";

    char* trailing_slash = strrchr(argv[1], '/');
    if (trailing_slash) {
        *trailing_slash = '\0';
        output_name = String(trailing_slash + 1);
        prepend = String(argv[1]);
        prepend += "/";
    }

    String output_header = prepend;
    output_header += output_name;
    output_header += "_token_type.h";

    String output_parser = prepend;
    output_parser += "generic_";
    output_parser += output_name;
    output_parser += "_parser.h";

    Generator generator(state_table, identifiers, token_types, literals, output_name);
    generator.generate_token_type_header(output_header);
    generator.generate_generic_parser(output_parser);

    if (munmap(contents, info.st_size) != 0) {
        perror("munmap");
        return 1;
    }

    return 0;
}