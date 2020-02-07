#include <regex.h>
#include <string.h>

#include "regex_graph.h"

extern "C" int regexec(const regex_t* __restrict regex, const char* __restrict str, size_t nmatch, regmatch_t __restrict dest_matches[],
                       int eflags) {
    if (!regex->__re_compiled_data) {
        return REG_NOMATCH;
    }

    RegexGraph* graph = static_cast<RegexGraph*>(regex->__re_compiled_data);
    auto matches = graph->do_match(str, eflags);
    if (!matches.size()) {
        return REG_NOMATCH;
    }

    for (size_t i = 0; i < nmatch; i++) {
        if (i < (size_t) matches.size()) {
            dest_matches[i] = matches[i];
        } else {
            dest_matches[i] = { -1, -1 };
        }
    }

    return 0;
}
