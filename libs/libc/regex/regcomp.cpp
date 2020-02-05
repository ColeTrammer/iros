#include <assert.h>
#include <bits/malloc.h>
#include <regex.h>

#include "bre_lexer.h"
#include "bre_parser.h"
#include "bre_value.h"

extern "C" int regcomp(regex_t* __restrict regex, const char* __restrict str, int cflags) {
    assert(!(cflags & REG_EXTENDED));

    BRECompiledData* compiled_data = static_cast<BRECompiledData*>(malloc(sizeof(BRECompiledData)));
    if (!compiled_data) {
        return REG_ESPACE;
    }

    int error = 0;
    new (compiled_data) BRECompiledData(str);
    if (!compiled_data->lexer.lex()) {
        error = compiled_data->lexer.error_code();
        goto regcomp_error;
    }

    if (!compiled_data->parser.parse()) {
        error = compiled_data->parser.error_code();
        goto regcomp_error;
    }

    regex->re_nsub = compiled_data->lexer.num_sub_expressions();
    regex->__re_compiled_data = static_cast<void*>(compiled_data);
    return 0;

regcomp_error:
    compiled_data->~BRECompiledData();
    free(compiled_data);
    regex->__re_compiled_data = nullptr;
    return error;
}