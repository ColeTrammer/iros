#pragma once

#include <ccpp/bits/time_t.h>

__CCPP_BEGIN_DECLARATIONS

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

__CCPP_END_DECLARATIONS
