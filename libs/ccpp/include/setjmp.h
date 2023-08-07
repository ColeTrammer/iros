#pragma once

#include <ccpp/bits/config.h>

#include __CCPP_ARCH_PATH(jmpbuf.h)

__CCPP_BEGIN_DECLARATIONS

typedef struct __jmp_buf jmp_buf[1];

int setjmp(jmp_buf __env);
__CCPP_NORETURN void longjmp(jmp_buf __env, int __val);

__CCPP_END_DECLARATIONS
