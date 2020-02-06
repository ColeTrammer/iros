#include <assert.h>
#include <bits/malloc.h>
#include <regex.h>

#include "bre_graph.h"
#include "bre_lexer.h"
#include "bre_parser.h"
#include "bre_value.h"

extern "C" int regcomp(regex_t* __restrict regex, const char* __restrict str, int cflags) {
    assert(!(cflags & REG_EXTENDED));

    int error = 0;
    BREGraph* compiled = nullptr;
    BRELexer lexer(str, cflags);
    BREParser parser(lexer, cflags);

    if (!lexer.lex()) {
        error = lexer.error_code();
        goto regcomp_error;
    }

    if (!parser.parse()) {
        error = parser.error_code();
        goto regcomp_error;
    }

    if (!parser.result().is<BRE>()) {
        error = REG_BADPAT;
        goto regcomp_error;
    }

    compiled = static_cast<BREGraph*>(malloc(sizeof(BREGraph)));
    if (!compiled) {
        error = REG_ESPACE;
        goto regcomp_error;
    }

    new (compiled) BREGraph(parser.result().as<BRE>(), cflags);

    regex->re_nsub = lexer.num_sub_expressions();
    regex->__re_compiled_data = static_cast<void*>(compiled);
    return 0;

regcomp_error:
    regex->__re_compiled_data = nullptr;
    return error;
}