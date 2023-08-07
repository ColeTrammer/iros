#pragma once

#include <ccpp/bits/config.h>

__CCPP_BEGIN_DECLARATIONS

struct __jmp_buf {
    // The SYS-V ABI specifies 6 callee-saved registers, but we must also include the stack and instruction pointers.
    unsigned long __registers[8];
};

__CCPP_END_DECLARATIONS
