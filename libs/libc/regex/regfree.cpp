#include <bits/malloc.h>
#include <regex.h>

#include "bre_graph.h"

extern "C" void regfree(regex_t* regex) {
    if (regex->__re_compiled_data) {
        BREGraph* data = reinterpret_cast<BREGraph*>(regex->__re_compiled_data);
        data->~BREGraph();
        free(data);
        regex->__re_compiled_data = nullptr;
    }

    regex->re_nsub = 0;
}
