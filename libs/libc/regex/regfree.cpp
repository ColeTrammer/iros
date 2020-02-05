#include <bits/malloc.h>
#include <regex.h>

#include "bre_parser.h"

extern "C" void regfree(regex_t* regex) {
    if (regex->__re_compiled_data) {
        BRECompiledData* data = reinterpret_cast<BRECompiledData*>(regex->__re_compiled_data);
        data->~BRECompiledData();
        free(data);
        regex->__re_compiled_data = nullptr;
    }

    regex->re_nsub = 0;
}
