#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../../libs/libc/include/wordexp.h"
#endif /* USERLAND_NATIVE */

#include "sh_state.h"

ShState& ShState::the() {
    static ShState* state;
    if (!state) {
        state = new ShState();
    }

    assert(state);
    return *state;
}

int ShState::flags_for_wordexp() const {
    int flags = WRDE_SPECIAL;
    flags |= option_noglob() ? WRDE_NOGLOB : 0;
    flags |= option_nounset() ? WRDE_UNDEF : 0;
    return flags;
}

bool ShState::process_arg(const char* s) {
    if (s[0] != '-' && s[1] != '+') {
        return false;
    }

#undef __ENUMERATE_OPTIONS
#define __ENUMERATE_OPTIONS(ss, c) \
    case c:                        \
        m_##ss = s[0] == '-';      \
        break;

    switch (s[1]) {
        ENUMERATE_OPTIONS
        default:
            return false;
    }

    return true;
}

void ShState::process_option(const char* s, bool to_set) {
#undef __ENUMERATE_OPTIONS
#define __ENUMERATE_OPTIONS(ss, c) \
    if (strcmp(s, #ss) == 0) {     \
        m_##ss = to_set;           \
    }

    ENUMERATE_OPTIONS
}

void ShState::dump() const {
#undef __ENUMERATE_OPTIONS
#define __ENUMERATE_OPTIONS(ss, cc)        \
    {                                      \
        char c = '+';                      \
        if (m_##ss) {                      \
            c = '-';                       \
        }                                  \
        fprintf(stderr, "%c%s\n", c, #ss); \
    }

    ENUMERATE_OPTIONS
}
