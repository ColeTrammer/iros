#define GENERIC_BASIC_PARSER_DEBUG

#include <stdio.h>

#include "../../libs/libc/regex/bre_lexer.h"
#include "../../libs/libc/regex/bre_parser.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return 1;
    }

    BRELexer lexer(argv[1], 0);
    if (!lexer.lex()) {
        printf("Failed: %d\n", lexer.error_code());
        return 1;
    }

    lexer.tokens().for_each([](const BRELexer::Token& token) {
        printf("%s '%s'\n", BREParser::token_type_to_string(token.type()), String(token.value().as<TokenInfo>().text).string());
    });

    BREParser parser(lexer, 0);
    if (!parser.parse()) {
        printf("Failed: %d\n", parser.error_code());
        return 1;
    }

    dump(parser.result());
    return 0;
}