#include <assert.h>
#include <fcntl.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "item_set.h"
#include "lexer.h"
#include "rule.h"

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

    *strrchr(argv[1], '.') = '\0';

    String output_name = argv[1];
    String output_header = output_name;
    output_header += "_token_type.h";

    fprintf(stderr, "Writing token types to %s.\n", output_header.string());

    int ofd = open(output_header.string(), O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (ofd == -1) {
        perror("opening output header");
        return 1;
    }

    FILE* token_type_header = fdopen(ofd, "w");
    fprintf(token_type_header, "#pragma once\n\n");
    fprintf(token_type_header, "#define ENUMERATE_%s_TOKEN_TYPES \\\n", output_name.to_upper_case().string());

    StringView* start_name = nullptr;

    Vector<StringView> token_types;

    for (int i = 0; i < tokens.size(); i++) {
        auto& token = tokens[i];
        switch (token.type()) {
            case TokenType::TokenWord:
                fprintf(token_type_header, "__ENUMERATE_%s_TOKEN_TYPES(%s) \\\n", output_name.string(), String(token.text()).string());
                token_types.add(token.text());
            case TokenType::TokenTokenMarker:
                continue;
            case TokenType::TokenStartMarker:
                start_name = &tokens[i + 1].text();
                goto done;
            default:
                assert(false);
        }
    }

done:
    fprintf(token_type_header, "\nenum class %sTokenType {\n", output_name.to_title_case().string());
    fprintf(token_type_header, "#define _(s) s\n");
    fprintf(token_type_header, "#undef __ENUMERATE_%s_TOKEN_TYPES\n", output_name.to_upper_case().string());
    fprintf(token_type_header, "#define __ENUMERATE_%s_TOKEN_TYPES(t) _(##t),\n", output_name.to_upper_case().string());
    fprintf(token_type_header, "ENUMERATE_%s_TOKEN_TYPES End\n", output_name.to_upper_case().string());
    fprintf(token_type_header, "#undef _\n");
    fprintf(token_type_header, "#undef __ENUMERATE_%s_TOKEN_TYPES\n", output_name.to_upper_case().string());
    fprintf(token_type_header, "};\n");

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
                default:
                    assert(false);
            }
        }
    }

    Rule* start_rule = nullptr;
    rules.for_each([&](auto& rule) {
        if (rule.name() == *start_name) {
            start_rule = &rule;
        }
    });

    auto sets = ItemSet::create_item_sets(*start_rule, rules, token_types);
    sets.for_each([&](auto& set) {
        fprintf(stderr, "%s\n", set->stringify().string());
    });

    if (fclose(token_type_header) != 0) {
        perror("fclose");
        return 1;
    }

    if (munmap(contents, info.st_size) != 0) {
        perror("munmap");
        return 1;
    }

    return 0;
}