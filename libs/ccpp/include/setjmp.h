#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

struct __jmp_buf {
    long __registers[6];
};

typedef struct __jmp_buf jmp_buf[1];

int setjmp(jmp_buf __env);
void longjmp(jmp_buf __env, int __val);

__CCPP_END_DECLARATIONS
