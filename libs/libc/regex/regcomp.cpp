#include <assert.h>

#ifdef USERLAND_NATIVE
#include <stdlib.h>
#include "../include/regex.h"
#else
#include <bits/malloc.h>
#include <regex.h>
#endif /* USERLAND_NATIVE */

#include "regex_graph.h"
#include "regex_lexer.h"
#include "regex_parser.h"
#include "regex_value.h"

extern "C" int regcomp(regex_t* __restrict regex, const char* __restrict str, int cflags) {
    int error = 0;
    RegexGraph* compiled = nullptr;
    RegexLexer lexer(str, cflags);
    RegexParser parser(lexer, cflags);

    if (!lexer.lex()) {
        error = lexer.error_code();
        goto regcomp_error;
    }

    if (!parser.parse()) {
        error = parser.error_code();
        goto regcomp_error;
    }

    if (!parser.result().is<ParsedRegex>()) {
        error = REG_BADPAT;
        goto regcomp_error;
    }

    compiled = static_cast<RegexGraph*>(malloc(sizeof(RegexGraph)));
    if (!compiled) {
        error = REG_ESPACE;
        goto regcomp_error;
    }

    new (compiled) RegexGraph(parser.result().as<ParsedRegex>(), cflags, lexer.num_sub_expressions());
    if (!compiled->compile()) {
        error = compiled->error_code();
        goto regcomp_error_after_allocation;
    }

    regex->re_nsub = lexer.num_sub_expressions();
    regex->__re_compiled_data = static_cast<void*>(compiled);
    return 0;

regcomp_error_after_allocation:
    compiled->~RegexGraph();
    free(compiled);

regcomp_error:
    regex->__re_compiled_data = nullptr;
    return error;
}