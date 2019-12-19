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
                StringView real_name = literal_to_token(token.text());
                if (!token_types.includes(real_name))
                    token_types.add(real_name);
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
    int num = 0;

    for (int i = 0; i < tokens.size(); i++) {
        auto& token = tokens.get(i);
        if (token.type() == TokenType::TokenPercentPercent) {
            start = true;
            continue;
        }

        if (start) {
            switch (token.type()) {
                case TokenType::TokenWord:
                    if (rule_name) {
                        rule.components().add(token.text());
                    } else {
                        rule_name = &token.text();
                    }
                    break;
                case TokenType::TokenColon:
                    assert(rule_name);
                    rule.name() = *rule_name;
                    break;
                case TokenType::TokenSemicolon:
                    rule_name = nullptr;
                    // Fall through
                case TokenType::TokenPipe:
                    rule.set_number(num++);
                    rules.add(rule);
                    rule.components().clear();
                    break;
                case TokenType::TokenLiteral: {
                    if (rule_name) {
                        rule.components().add(literal_to_token(token.text()));
                    } else {
                        fprintf(stderr, "Literal cannot be left hand size of rule\n");
                        exit(1);
                    }
                    break;
                }
                default:
                    assert(false);
            }
        }
    }

    fprintf(stderr, "\n");

    Rule* start_rule = nullptr;
    rules.for_each([&](auto& rule) {
        if (rule.name() == *start_name) {
            start_rule = &rule;
        }
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
    dummy_start.components().add(start_rule->name());
    dummy_start.set_number(0);

    rules.for_each([&](auto& rule) {
        rule.set_number(rule.number() + 1);
    });

    rules.add(dummy_start);
    start_rule = &rules.last();

    identifiers.add("__start");
    fprintf(stderr, "Added __start rule: %s\n", dummy_start.stringify().string());

    auto sets = ItemSet::create_item_sets(*start_rule, rules, token_types);
    sets.for_each([&](auto& set) {
        fprintf(stderr, "%s\n", set->stringify().string());
    });

    ExtendedGrammar extended_grammar(sets, token_types);
    fprintf(stderr, "\n%s\n", extended_grammar.stringify().string());

    StateTable state_table(extended_grammar, identifiers, token_types);
    fprintf(stderr, "%s\n", state_table.stringify().string());

    *strrchr(argv[1], '.') = '\0';
    String output_name = argv[1];

    String output_header = output_name;
    output_header += "_token_type.h";

    String output_parser = "generic_";
    output_parser += output_name;
    output_parser += "_parser.h";

    Generator generator(state_table, identifiers, token_types, output_name);
    generator.generate_token_type_header(output_header);
    generator.generate_generic_parser(output_parser);

    if (munmap(contents, info.st_size) != 0) {
        perror("munmap");
        return 1;
    }

    return 0;
}