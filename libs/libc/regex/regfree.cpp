#include <bits/malloc.h>
#include <regex.h>

#include "regex_graph.h"

extern "C" void regfree(regex_t* regex) {
    if (regex->__re_compiled_data) {
        RegexGraph* data = reinterpret_cast<RegexGraph*>(regex->__re_compiled_data);
        data->~RegexGraph();
        free(data);
        regex->__re_compiled_data = nullptr;
    }

    regex->re_nsub = 0;
}
